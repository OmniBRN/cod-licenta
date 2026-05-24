#include "core/simulation.hpp"
#include "core/behaviour.hpp"
#include "core/routing.hpp"
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

    do_spawning();

    rebuild_lane_index();

    for (size_t i = 0; i < m_cars.size(); ++i) {
        auto L = find_leader(i);
        Meters gap = L.exists ? L.gap : std::numeric_limits<Meters>::infinity();
        MetersPerSec dv = L.exists ? (m_cars[i].speed - L.lead_speed) : 0.0;
        step_car(m_cars[i], gap, dv);
    }

    retire_at_sinks();

    enforce_conservation();

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

    c.accel = idm_accel(bp, c.speed, gap, dv);
    c.speed = std::max(0.0, c.speed + c.accel * TICK_DT);
    c.offset = c.offset + c.speed * TICK_DT;

    advance_edges(c);
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

    if (obey_lights) {
        auto lit = m_lights.find(c.current_edge);
        if (lit != m_lights.end() && lit->second.is_red()) {
            Meters stop_line = m_net.edge_length(c.current_edge);
            Meters gap_to_stop = (stop_line - c.offset) - CAR_LENGTH;
            LeaderInfo stop{std::max(gap_to_stop, 0.0), 0.0, true};
            if (!real.exists || stop.gap < real.gap)
                return stop;
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
        proto.current_lane = 0;
        proto.offset = 0.0;
        proto.speed = 0.0;
        proto.route = std::move(*path);
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
           o.current_lane == c.current_edge &&
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

}