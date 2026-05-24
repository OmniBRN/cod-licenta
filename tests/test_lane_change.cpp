#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"

using namespace sim;

TEST_CASE("mandatory lane change occurs", "[lanechange]") {
    Simulation s = simtest::make_line_sim(500.0, 2);

    Car proto;
    proto.current_edge = 0;
    proto.current_lane = 1;
    proto.intended_lane = 0;
    proto.offset = 50.0;
    proto.speed = 10.0;
    proto.route = {0};
    proto.route_index = 0;
    proto.profile_id = 0;
    CarId id = s.spawn_car(proto);
    
    for(int i=0; i<200; ++i) s.tick();

    const Car* c = nullptr;
    for(const auto& x: s.cars()) if (x.id == id) c = &x;
    REQUIRE(c != nullptr);
    REQUIRE(c->current_lane == 0);
}