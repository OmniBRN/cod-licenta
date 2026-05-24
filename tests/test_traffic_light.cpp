#include <catch2/catch_test_macros.hpp>
#include "core/traffic_light.hpp"

using namespace sim;

TEST_CASE("traffic light cycles correctly", "[traffic_light]") {
    TrafficLight tl;
    tl.color = LightColor::Green;
    tl.phase_ticks = 0;
    tl.green_duration = 5;
    tl.yellow_duration = 2;
    tl.red_duration = 5;

    for (TickT i = 0; i < 5; ++i) tl.advance();
    REQUIRE(tl.is_yellow());

    for (TickT i = 0; i < 2; ++i) tl.advance();
    REQUIRE(tl.is_red());

    for (TickT i = 0; i < 5; ++i) tl.advance();
    REQUIRE(tl.is_green());

}