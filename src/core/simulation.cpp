#include "core/simulation.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>

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
    ++m_tick;
    enforce_conservation();
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

}