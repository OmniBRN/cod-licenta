#include "core/simulation.hpp"
#include "core/behaviour.hpp"
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
    rebuild_lane_index();

    for (size_t i = 0; i < m_cars.size(); ++i) {
        auto L = find_leader(i);
        Meters gap = L.exists ? L.gap : std::numeric_limits<Meters>::infinity();
        MetersPerSec dv = L.exists ? (m_cars[i].speed - L.lead_speed) : 0.0;
        step_car(m_cars[i], gap, dv);
    }
    enforce_conservation();
    ++m_tick;
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

Simulation::LeaderInfo Simulation::find_leader(size_t car_idx) const {
    const Car& c = m_cars[car_idx];
    const auto& lane = m_lane_cars[c.current_edge][c.current_lane];

    auto it = std::upper_bound(lane.begin(), lane.end(), c.offset, [&](Meters off, size_t other){
        return off < m_cars[other].offset;
    });

    if (it == lane.end()) {
        return {std::numeric_limits<Meters>::infinity(), 0.0, false};
    }
    const Car& lead = m_cars[*it];
    Meters gap = (lead.offset - c.offset) - CAR_LENGTH;
    return {std::max(gap, 0.0), lead.speed, true};

}

}