#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/behaviour.hpp"
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