#pragma once
#include "core/network.hpp"
#include "core/car.hpp"
#include "core/behaviour.hpp"
#include "rng/prng.hpp"
#include <vector>
#include <iosfwd>
namespace sim {
class Simulation {
public:

    explicit Simulation(Network net);

    void tick();

    const Network& network() const { return m_net; }
    const std::vector<Car>& cars() const { return m_cars; }
    TickT current_tick() const { return m_tick; }

    CarId add_car_at(EdgeId edge, LaneIdx lane, Meters offset, ProfileId profile);

    void dump_state(std:: ostream& os) const;

    ProfileId register_profile(BehaviourProfile p);

    void seed(uint64_t master) { m_rng.seed_all(master); }

private:

    Network m_net;
    std::vector<Car> m_cars;
    std::vector<BehaviourProfile> m_profiles;

    TickT m_tick = 0;
    size_t m_cars_spawned = 0;
    size_t m_cars_exited = 0;
    CarId m_next_car_id = 1;

    RngBank m_rng;

    void enforce_conservation() const;

};
}