#include "core/simulation.hpp"
#include "core/behaviour.hpp"
#include "core/routing.hpp"
#include "core/network.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>
#include <limits>
#include <algorithm>

namespace sim {

Simulation::Simulation(Network net): m_net(std::move(net)) {
    m_profiles.push_back(default_profile());
}

ProfileId Simulation::register_profile(BehaviourProfile p) {
    m_profiles.push_back(std::move(p));
    return static_cast<ProfileId>(m_profiles.size() - 1);
}

CarId Simulation::add_car_at(EdgeId edge, LaneIdx lane, Meters offset, ProfileId profile) {

    Car c;
    c.id = m_next_car_id++;
    c.profile_id = profile;
    c.current_edge = edge;
    c.current_lane = lane;
    c.offset = offset;
    c.intended_lane = lane;
    c.spawn_tick = m_tick;
    m_cars.push_back(c);
    ++m_cars_spawned;
    return c.id;

}

void Simulation::tick() {

    size_t exited_before = m_cars_exited;

    do_spawning();

    tick_lights();

    update_intended_lanes();

    rebuild_lane_index();

    for (size_t i = 0; i < m_cars.size(); ++i) {
        const BehaviourProfile& bp = m_profiles[m_cars[i].profile_id];
        bool obey = m_rng.compliance.bernoulli(bp.traffic_law_compliance);
        if (!obey) {
            auto lit = m_lights.find(m_cars[i].current_edge);
            if (lit != m_lights.end() && lit->second.is_red())
                ++m_cars[i].violations;
        }
        auto L = find_leader(i, obey);
        Meters gap = L.exists ? L.gap : std::numeric_limits<Meters>::infinity();
        MetersPerSec dv = L.exists ? (m_cars[i].speed - L.lead_speed) : 0.0;
        step_car(m_cars[i], gap, dv);
    }

    do_lane_change();

    apply_junction_approach();

    for (auto& c: m_cars) {
        c.speed = std::max(0.0, c.speed + c.accel * TICK_DT);
        c.trip_distance += c.speed * TICK_DT;
        c.offset = c.offset + c.speed * TICK_DT;
        advance_edges(c);
    }

    retire_at_sinks();

    enforce_conservation();

    size_t m_tick_throughput = m_cars_exited - exited_before;

    if (m_writer && m_tick >= m_warmup_ticks) {
        io::TickRecord tk;
        tk.tick = m_tick;
        tk.in_network = m_cars.size();
        tk.throughput = m_tick_throughput;
        m_writer->write_tick(tk);
    }

    ++m_tick;
}

void Simulation::retire_at_sinks() {
    for (size_t i = 0; i<m_cars.size();) {
        const Car& c = m_cars[i];
        bool last_edge = (c.route_index + 1 >= c.route.size());
        Meters L = m_net.edge_length(c.current_edge);
        NodeId end_node = m_net.edges[c.current_edge].to;
        bool at_sink = last_edge && c.offset >= L && m_net.nodes[end_node].kind == NodeKind::Sink;

        if (at_sink) {
            if (m_writer && m_tick >= m_warmup_ticks) {
                const Car& car = m_cars[i];
                io::TripRecord tr;
                tr.car_id = car.id; 
                tr.spawn_tick = car.spawn_tick;
                tr.exit_tick = m_tick;
                tr.trip_distance = car.trip_distance;
                double dur_s = static_cast<double>(m_tick - car.spawn_tick) * TICK_DT;
                tr.avg_speed = (dur_s > 0.0) ? car.trip_distance / dur_s : 0.0;
                tr.lane_changes = car.lane_changes;
                tr.violations = car.violations;
                tr.archetype = (car.profile_id < m_profiles.size()) 
                                ? m_profiles[car.profile_id].name : "unknown";
                m_writer -> write_trip(tr);
            }
            m_cars[i] = std::move(m_cars.back());
            m_cars.pop_back();
            ++m_cars_exited;
        } else {
            ++i;
        }

    }
}

void Simulation::enforce_conservation() const {
    size_t in_network = m_cars.size();
    if (m_cars_spawned != m_cars_exited + in_network) {
        throw std::logic_error(
            "conservation broken: spawned != exited + in_network"
        );
    }
}

void Simulation::dump_state(std::ostream& os) const {
    os 
    << "tick=" << m_tick << '\n'
    << "spawned=" << m_cars_spawned << '\n'
    << "exited=" << m_cars_exited << '\n'
    << "in_net=" << m_cars.size() << '\n';

    for (const auto& c: m_cars) {
        os
        << "car " << c.id << '\n'
        << "\tedge=" << c.current_edge << '\n'
        << "\tlane=" << static_cast<int>(c.current_lane) << '\n'
        << "\toffset=" << c.offset << '\n'
        << "\tv=" << c.speed << '\n';
    }
    os << "\n";
}

CarId Simulation::spawn_car(Car proto) {
    proto.id = m_next_car_id++;
    proto.spawn_tick = m_tick;
    m_cars.push_back(std::move(proto));
    ++m_cars_spawned;
    return m_cars.back().id;
}

void Simulation::step_car(Car& c, Meters gap, MetersPerSec dv) {
    const BehaviourProfile& bp = m_profiles[c.profile_id];

    constexpr double KMH_TO_MS = 10.0 / 36.0;
    MetersPerSec limit = m_net.edges[c.current_edge].speed_limit;
    MetersPerSec cap = limit + bp.aggressiveness * (15.0 * KMH_TO_MS);

    if (bp.desired_speed > cap) {
        BehaviourProfile capped = bp;
        capped.desired_speed = cap;
        c.accel = idm_accel(capped, c.speed, gap, dv);
    } else {
        c.accel = idm_accel(bp, c.speed, gap, dv);
    }
}

void Simulation::advance_edges(Car& c) {
    Meters L = m_net.edge_length(c.current_edge);
    while (c.offset >= L) {
        if (c.route_index + 1 >= c.route.size()) {
            c.offset = L;
            return;
        }
        c.offset -= L;
        c.route_index += 1;
        c.current_edge = c.route[c.route_index];
        c.current_lane = std::min<LaneIdx>(c.current_lane, m_net.edges[c.current_edge].lanes_forward - 1);
        c.is_changing_lane = false;
        L = m_net.edge_length(c.current_edge);
    }
}

void Simulation::rebuild_lane_index() {
    const size_t E = m_net.edges.size();
    m_lane_cars.assign(E, {});
    for (size_t e = 0; e < E; ++e)
        m_lane_cars[e].assign(m_net.edges[e].lanes_forward, {});
    
    for (size_t i = 0; i < m_cars.size(); ++i) {
        const Car& c = m_cars[i];
        m_lane_cars[c.current_edge][c.current_lane].push_back(i);
        if (c.is_changing_lane &&
            c.lane_change_to < m_net.edges[c.current_edge].lanes_forward &&
            c.lane_change_to != c.current_lane) {
            m_lane_cars[c.current_edge][c.lane_change_to].push_back(i);
        }
    }
    for (auto& per_edge : m_lane_cars) 
        for (auto& lane: per_edge)
            std::sort(lane.begin(), lane.end(), 
                [&](size_t a, size_t b) {
                    return m_cars[a].offset < m_cars[b].offset;
                }
            );

}

Simulation::LeaderInfo Simulation::find_leader(size_t car_idx, bool obey_lights) const {
    const Car& c = m_cars[car_idx];
    const auto& lane = m_lane_cars[c.current_edge][c.current_lane];

    auto it = std::upper_bound(lane.begin(), lane.end(), c.offset, [&](Meters off, size_t other){
        return off < m_cars[other].offset;
    });

    LeaderInfo real = (it==lane.end())
    ? LeaderInfo{std::numeric_limits<Meters>::infinity(), 0.0, false}
    : [&](){
        const Car& lead = m_cars[*it];
        Meters gap = (lead.offset - c.offset) - CAR_LENGTH;
        return LeaderInfo{std::max(gap, 0.0), lead.speed, true};
    }();

    if (c.route_index + 1 < c.route.size()) {
        EdgeId next_edge = c.route[c.route_index + 1];
        LaneIdx next_lane = std::min<LaneIdx>(c.current_lane, m_net.edges[next_edge].lanes_forward - 1);
        const auto& nxt = m_lane_cars[next_edge][next_lane];
        if (!nxt.empty()) {
            const Car& ahead = m_cars[nxt.front()];
            Meters edge_remaining = m_net.edge_length(c.current_edge) - c.offset;
            Meters gap = std::max(0.0, edge_remaining + ahead.offset - CAR_LENGTH);
            if (!real.exists || gap < real.gap)
                real = {gap, ahead.speed, true};
        }
    }

    if (obey_lights) {
        auto lit = m_lights.find(c.current_edge);
        if (lit != m_lights.end() && lit->second.is_red()) {
            Meters stop_line = m_net.edge_length(c.current_edge) - CAR_LENGTH;
            if (c.offset < stop_line) {
                Meters gap_to_stop = stop_line - c.offset;
                LeaderInfo stop{std::max(gap_to_stop, 0.0), 0.0, true};
                if (!real.exists || stop.gap < real.gap)
                    return stop;
            }
        }
    }

    return real;


}

void Simulation::set_sources(std::vector<SourceSpec> s) {
    while (m_profiles.size() < 5) {
        m_profiles.resize(0);
        m_profiles.push_back(ideal_profile()); // 0
        m_profiles.push_back(cautious_profile()); // 1
        m_profiles.push_back(normal_profile()); // 2
        m_profiles.push_back(aggresive_profile()); // 3
        m_profiles.push_back(opportunist_profile()); // 4
    }
    m_spawner = Spawner(std::move(s));
}

void Simulation::do_spawning(){
    auto pending = m_spawner.sample_for_tick(m_rng.arrivals, m_rng.archetypes, m_rng.destinations);

    for(auto& p : pending) {
        auto path = shortest_path_edges(m_net, p.source_node, p.destionation);
        if (!path) {
            ++m_failed_spawns;
            continue;
        }
        Car proto;
        proto.current_edge = (*path)[0];
        proto.offset = 0.0;
        proto.speed = 0.0;
        proto.route = std::move(*path);
        proto.current_lane = static_cast<LaneIdx>(
            m_cars_spawned % m_net.edges[proto.current_edge].lanes_forward);
        proto.intended_lane = compute_intended_lane(m_net, proto);
        proto.route_index = 0;
        proto.profile_id = p.profile;
        proto.target_exit = p.destionation;

        if (spawn_point_blocked(proto)) {
            ++m_failed_spawns;
            continue;
        }
        spawn_car(std::move(proto));
    }
}

bool Simulation::spawn_point_blocked(const Car& c) const {
    for(const auto& o : m_cars)
        if(o.current_edge == c.current_edge &&
           o.current_lane == c.current_lane &&
           o.offset < CAR_LENGTH + 2.0)
            return true;
    return false;
}

void Simulation::update_intended_lanes() {
    for (auto& c : m_cars)
        c.intended_lane = compute_intended_lane(m_net, c);
}

void Simulation::set_lights(std::unordered_map<EdgeId, TrafficLight> lights) {
    m_lights = std::move(lights);
}

void Simulation::tick_lights() {
    for (auto& [eid, tl] : m_lights) tl.advance();
}

void Simulation::apply_junction_approach() {
    constexpr MetersPerSec JUNCTION_SPEED = 5.0;
    for (auto& c : m_cars) {
        if (c.speed <= JUNCTION_SPEED) continue;
        if (c.route_index + 1 >= c.route.size()) continue;
        NodeId dst = m_net.edges[c.current_edge].to;
        if (m_net.nodes[dst].kind != NodeKind::Junction) continue;

        EdgeId nxt = c.route[c.route_index + 1];
        Vec2 cur_dir = m_net.direction_at(c.current_edge, c.current_lane,
                                          m_net.edge_length(c.current_edge));
        Vec2 nxt_dir = m_net.direction_at(nxt, 0, 0.0);
        if (cur_dir.x * nxt_dir.x + cur_dir.y * nxt_dir.y >= 0.7) continue;

        const BehaviourProfile& bp = m_profiles[c.profile_id];
        double braking_dist = (c.speed * c.speed - JUNCTION_SPEED * JUNCTION_SPEED)
                              / (2.0 * bp.comfort_brake);
        Meters dist = m_net.edge_length(c.current_edge) - c.offset;
        if (dist > braking_dist) continue;

        double needed = (c.speed * c.speed - JUNCTION_SPEED * JUNCTION_SPEED)
                        / (2.0 * std::max(dist, 1.0));
        c.accel = std::min(c.accel, -needed);
    }
}

bool Simulation::gap_accept(size_t car_idx, LaneIdx target) const {
    const Car& c = m_cars[car_idx];
    EdgeId e = c.current_edge;
    if (target >= m_net.edges[e].lanes_forward) return false;

    constexpr Meters SAFE_FRONT = 10.0;
    constexpr Meters SAFE_REAR = 6.0;

    for (size_t idx: m_lane_cars[e][target]) {
        const Car& o = m_cars[idx];
        if (o.offset > c.offset) {
            if ((o.offset - c.offset) < SAFE_FRONT + CAR_LENGTH) return false;
        } else {
            if ((c.offset - o.offset) < SAFE_REAR + CAR_LENGTH) return false;
        }
    }
    return true;
}

bool Simulation::in_intesection(const Car& c) const {
    const Edge& e = m_net.edges[c.current_edge];
    bool both_junction = m_net.nodes[e.from].kind == NodeKind::Junction &&
                         m_net.nodes[e.to].kind == NodeKind::Junction;
    return both_junction && e.length < 20.0;
}

void Simulation::do_lane_change() {
    for (auto& c : m_cars) {
        if (!c.is_changing_lane) continue;
        if (c.lane_change_ticks_remaining == 0) {
            c.current_lane = c.lane_change_to;
            c.is_changing_lane = false;
        } else {
            --c.lane_change_ticks_remaining;
        }
    }

    for(size_t i = 0; i<m_cars.size(); ++i) {
        Car& c = m_cars[i];

        if (c.lane_change_cooldown > 0) --c.lane_change_cooldown;

        if (c.is_changing_lane) continue;

        if (in_intesection(c)) continue;

        uint8_t max_lane = m_net.edges[c.current_edge].lanes_forward-1;

        if (c.current_lane != c.intended_lane) {
            if(gap_accept(i, c.intended_lane)) {
                c.is_changing_lane = true;
                c.lane_change_from = c.current_lane;
                c.lane_change_to = c.intended_lane;
                c.lane_change_ticks_remaining = LANE_CHANGE_TICKS;
                ++c.lane_changes;
                continue;
            }

            int step = (c.intended_lane > c.current_lane) ? 1 : -1;
            LaneIdx next = static_cast<LaneIdx>(c.current_lane + step);
            if (gap_accept(i, next)) {
                c.is_changing_lane = true;
                c.lane_change_from = c.current_lane;
                c.lane_change_to = next;
                c.lane_change_ticks_remaining = LANE_CHANGE_TICKS;
                ++c.lane_changes;
                continue;
            }
        }

        if (c.lane_change_cooldown > 0) continue;

        const BehaviourProfile& bp = m_profiles[c.profile_id];
        if (!m_rng.lane_change.bernoulli(bp.aggressiveness * 0.05)) continue;

        const auto& cur_vec = m_lane_cars[c.current_edge][c.current_lane];
        auto cur_it = std::upper_bound(cur_vec.begin(), cur_vec.end(), c.offset,
            [&](Meters off, size_t idx){ return off < m_cars[idx].offset; });
        MetersPerSec cur_leader_spd = (cur_it != cur_vec.end())
            ? m_cars[*cur_it].speed
            : std::numeric_limits<MetersPerSec>::infinity();

        if (cur_leader_spd >= bp.desired_speed * 0.9) continue;

        for(int delta:{-1, 1}) {
            int target_int = static_cast<int>(c.current_lane) + delta;
            if (target_int < 0 || target_int > static_cast<int>(max_lane)) continue;
            LaneIdx target = static_cast<LaneIdx>(target_int);
            if (c.intended_lane != c.current_lane) continue;

            auto route_accepts = [&]() -> bool {
                if (c.route_index + 1 >= c.route.size()) return true;
                EdgeId cur_edge = c.route[c.route_index];
                EdgeId nxt = c.route[c.route_index + 1];
                bool any_rule = false;
                for (const auto& r : m_net.turn_rules) {
                    if (r.from_edge == cur_edge && r.to_edge == nxt) {
                        any_rule = true;
                        if (r.from_lane == target) return true;
                    }
                }
                return !any_rule;
            };
            if (!route_accepts()) continue;

            if (!gap_accept(i, target)) continue;

            const auto& tgt_vec = m_lane_cars[c.current_edge][target];
            auto tgt_it = std::upper_bound(tgt_vec.begin(), tgt_vec.end(), c.offset,
                [&](Meters off, size_t idx){ return off < m_cars[idx].offset; });
            MetersPerSec tgt_leader_spd = (tgt_it != tgt_vec.end())
                ? m_cars[*tgt_it].speed
                : std::numeric_limits<MetersPerSec>::infinity();

            constexpr MetersPerSec MIN_BENEFIT = 2.0;
            if (tgt_leader_spd < cur_leader_spd + MIN_BENEFIT) continue;
            if (tgt_leader_spd < c.speed) continue;

            c.is_changing_lane = true;
            c.lane_change_from = c.current_lane;
            c.lane_change_to = target;
            c.lane_change_ticks_remaining = LANE_CHANGE_TICKS;
            ++c.lane_changes;
            c.lane_change_cooldown = 30;
            break;
        }
    }
}

void Simulation::set_output_dir(const std::filesystem::path& out_dir, const std::string& stamp) {
    m_writer = std::make_unique<io::MetricsWriter>(out_dir, stamp);
}

void Simulation::set_warmup_ticks(TickT warmup) {
    m_warmup_ticks = warmup;
}

}