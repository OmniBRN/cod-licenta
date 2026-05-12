#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/network.hpp"

using namespace sim;
using Catch::Approx;

TEST_CASE("point at lane offset", "[geometry]") {
    Network net;
    net.nodes = {{0, {0,0}, NodeKind::Source},
                 {1, {100, 0}, NodeKind::Sink}};
    Edge e;
    e.id = 0; e.from = 0; e.to = 1;
    e.lanes_forward = 2;
    e.polyline = {{0,0}, {100, 0}};
    e.length = polyline_length(e.polyline);
    net.edges.push_back(e);

    Vec2 p0 = net.point_at(0, 0, 50.0);
    Vec2 p1 = net.point_at(0, 1, 50.0);

    REQUIRE(p0.x == Approx(50.0));
    REQUIRE(p1.x == Approx(50.0));
    REQUIRE(std::abs(p1.y) > std::abs(p0.y));
    REQUIRE(std::abs(p1.y - p0.y) == Approx(LANE_WIDTH));
}