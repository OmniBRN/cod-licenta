#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"
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

TEST_CASE("car stops at red light", "[traffic_light]") {
    Simulation s = simtest::make_line_sim(200.0);
    TrafficLight tl;
    tl.color = LightColor::Red;
    tl.green_duration = 99999;
    tl.yellow_duration = 99999;
    tl.red_duration = 99999;
    s.set_lights({{0, tl}});

    Car proto;
    proto.current_edge = 0;
    proto.current_lane = 0;
    proto.offset = 0.0;
    proto.speed = 0.0;
    proto.route = {0};
    proto.route_index = 0;
    proto.profile_id = 0;
    s.spawn_car(proto);

    for(int i=0; i<400; ++i) s.tick();

    REQUIRE(s.cars().size() == 1);
    const Car& c = s.cars()[0];
    REQUIRE(c.offset < 200.0);
    REQUIRE(c.speed < 0.5);
    
}