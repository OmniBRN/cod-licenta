#include <catch2/catch_test_macros.hpp>
#include "io/metrics_writer.hpp"

using namespace sim;
using namespace sim::io;

TEST_CASE("MetricsWriter creates CSV files", "[writer]") {
    namespace fs = std::filesystem;
    fs::path tmp = "data/output/test_tmp";
    fs::remove_all(tmp);

    {
        MetricsWriter w(tmp, "smoke");

        TripRecord tr;
        tr.car_id = 1;
        tr.spawn_tick = 0;
        tr.exit_tick = 100;
        tr.trip_distance = 200.0;
        tr.avg_speed = 2.0;
        tr.lane_changes = 3;
        tr.violations = 0;
        tr.archetype = "normal";

        w.write_trip(tr);

        TickRecord tk;
        tk.tick = 50;
        tk.in_network = 5;
        tk.throughput = 1;

        w.write_tick(tk);
    }

    REQUIRE(fs::exists(tmp / "trips_smoke.csv"));
    REQUIRE(fs::exists(tmp / "ticks_smoke.csv"));

    std::ifstream f(tmp / "trips_smoke.csv");
    std::string line;
    std::getline(f, line);
    std::getline(f, line);
    REQUIRE(line.find("normal") != std::string::npos);
    REQUIRE(line.find("200") != std::string::npos);

    fs::remove_all(tmp);
}