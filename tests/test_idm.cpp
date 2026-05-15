#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/behaviour.hpp"
#include "test_helpers.hpp"
#include <limits>

using namespace sim;
using Catch::Approx;

TEST_CASE("single car reaches desired speed", "[idm]") {
    BehaviourProfile bp;
    bp.desired_speed = 15.0;

    MetersPerSec v = 0.0;
    const Seconds dt = TICK_DT;
    const double inf = std::numeric_limits<double>::infinity();

    for (int i=0; i < 5000; ++i) {
        double a = idm_accel(bp, v, inf, 0.0);
        v = std::max(0.0, v + a * dt);
    }
    REQUIRE(v == Approx(bp.desired_speed).margin(0.05));
}

TEST_CASE("standstill behind stopped leader", "[idm]") {
    BehaviourProfile bp;
    double a = idm_accel(bp, 0.0, 2.0, 0.0);
    REQUIRE(a <= 0.0);
}

TEST_CASE("car cross edge boundary", "[sim]") {
    Simulation s = simtest::make_two_edge_sim(50.0);
    Car proto;
    proto.current_edge = 0; proto.offset = 49.9; proto.speed = 5.0;
    proto.route = {0, 1};
    proto.route_index = 0;
    proto.profile_id = 0;
    CarId id = s.spawn_car(proto);

    s.tick();

    const Car* cc = nullptr;
    for (const auto& x: s.cars()) if (x.id == id) cc = &x;
    REQUIRE(cc != nullptr);
    REQUIRE(cc->current_edge == 1);
    REQUIRE(cc->offset < 5.0);

}