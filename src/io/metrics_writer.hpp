#pragma once
#include "core/types.hpp"
#include <string>
#include <fstream>
#include <filesystem>

namespace sim::io {

struct TripRecord {
    CarId car_id;
    TickT spawn_tick;
    TickT exit_tick;
    Meters trip_distance;
    double avg_speed;
    uint32_t lane_changes;
    uint32_t violations;
    std::string archetype;
};

struct TickRecord {
    TickT tick;
    size_t in_network;
    size_t throughput;
};

class MetricsWriter {

public:
    MetricsWriter(const std::filesystem::path& out_dir, const std::string& stamp);

    void write_trip(const TripRecord& r);
    void write_tick(const TickRecord& r);

    void close();
    ~MetricsWriter() { close(); }

    MetricsWriter(const MetricsWriter&) = delete;
    MetricsWriter& operator=(const MetricsWriter&) = delete;

private:
    std::ofstream m_trips;
    std::ofstream m_ticks;
    bool m_open = false;
};

}