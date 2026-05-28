#pragma once
#include "core/network.hpp"
#include "core/car.hpp"
#include "core/behaviour.hpp"
#include "core/traffic_light.hpp"
#include "rng/prng.hpp"
#include "sim/spawner.hpp"
#include "io/metrics_writer.hpp"
#include <vector>
#include <iosfwd>
#include <unordered_map>
#include <memory>

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

    CarId spawn_car(Car proto);

    size_t spawned() const { return m_cars_spawned; }
    size_t exited() const { return m_cars_exited; }
    size_t in_network() const { return m_cars.size(); }

    void step_car (Car& c, Meters gap, MetersPerSec dv);

    void set_sources(std::vector<SourceSpec> s);

    void debug_leak() { ++m_cars_exited;}

    void set_lights(std::unordered_map<EdgeId, TrafficLight> lights);
    const std::unordered_map<EdgeId, TrafficLight>& lights() const { return m_lights;}

    void set_output_dir(const std::filesystem::path& out_dir, const std::string& stamp);
    void set_warmup_ticks(TickT);

    const BehaviourProfile& profile(ProfileId id) const { return m_profiles.at(id); }


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

    size_t m_failed_spawns = 0;

    void advance_edges(Car& c);

    struct LeaderInfo {
        Meters gap;
        MetersPerSec lead_speed;
        bool exists;
    };

    std::vector<std::vector<std::vector<size_t>>> m_lane_cars;

    void rebuild_lane_index();
    LeaderInfo find_leader(size_t car_idx, bool obey_lights = 1) const;

    Spawner m_spawner{ {} };
    void do_spawning();
    bool spawn_point_blocked(const Car& c) const;

    void retire_at_sinks();

    void update_intended_lanes();

    std::unordered_map<EdgeId, TrafficLight> m_lights;
    void tick_lights();
    void apply_junction_approach();

    bool gap_accept(size_t car_idx, LaneIdx target) const;
    bool in_intesection(const Car& c) const;
    void do_lane_change();

    std::unique_ptr<io::MetricsWriter> m_writer;
    TickT m_warmup_ticks = 0;
};
}