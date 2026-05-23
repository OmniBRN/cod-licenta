#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"

using namespace sim;

TEST_CASE("end-to-end: cross with Poisson sources", "[e2e]") {
    Simulation s = simtest::load_scenario("scenarios/networks/cross.json");
    s.seed(123);
    for (int i=0; i<6000; ++i) {
        s.tick();
    }
    REQUIRE(s.spawned() > 0);
    REQUIRE(s.exited() > 0);
    REQUIRE(s.spawned() == s.exited() + s.in_network());
}

TEST_CASE("invariant breaker throws", "[e2e]") {
    Simulation s = simtest::make_line_sim(100.0);
    Car p;
    p.current_edge = 0;
    p.route = {0};
    p.profile_id = 0;
    s.spawn_car(p);
    s.debug_leak();
    REQUIRE_THROWS_AS(s.tick(), std::logic_error);
}