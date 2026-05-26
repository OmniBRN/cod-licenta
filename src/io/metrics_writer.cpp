#include "io/metrics_writer.hpp"
#include <stdexcept>

namespace sim::io {

MetricsWriter::MetricsWriter(const std::filesystem::path& out_dir, const std::string& stamp) {
    std::filesystem::create_directories(out_dir);

    auto trip_path = out_dir / ("trips_" + stamp + ".csv");
    auto tick_path = out_dir / ("ticks_" + stamp + ".csv");

    m_trips.open(trip_path);
    m_ticks.open(tick_path);

    if (!m_trips || !m_ticks)
        throw std::runtime_error("MetricsWriter: cannot open output files");
    
    // CSV Headers
    m_trips << "car_id,spawn_tick,exit_tick"
            << "trip_distance_m,avg_speed_mps,lane_changes"
            << "violations, archetype\n";
    m_ticks << "tick, in_network, throughput\n";
    m_open = true;
}

void MetricsWriter::write_trip(const TripRecord& r) {
    if (!m_open) return;
    m_trips << r.car_id << ','
            << r.spawn_tick << ','
            << r.exit_tick << ','
            << r.trip_distance << ','
            << r.avg_speed << ','
            << r.lane_changes << ','
            << r.violations << ','
            << r.archetype << '\n';
}

void MetricsWriter::write_tick(const TickRecord& r) {
    if (!m_open) return;
    m_trips << r.tick << ','
            << r.in_network << ','
            << r.throughput << '\n';
}

void MetricsWriter::close() {
    if (!m_open) return;
    m_trips.close();
    m_ticks.close();
    m_open = false;
}

}