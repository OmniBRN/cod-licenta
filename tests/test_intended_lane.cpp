#include <catch2/catch_test_macros.hpp>
#include "core/network.hpp"
#include "core/car.hpp"
#include "core/routing.hpp"

using namespace sim;

TEST_CASE("intended lane from turn rule", "[routing]") {
    Network net;
    net.nodes = {
        {0, {0,0}, NodeKind::Source},
        {1, {100,0}, NodeKind::Junction},
        {2, {200,0}, NodeKind::Sink},
    };
    Edge e0;
    e0.id = 0;
    e0.from = 0;
    e0.to = 1;
    e0.lanes_forward = 2;
    e0.polyline = {{0,0}, {100,0}};
    e0.length = polyline_length(e0.polyline);

    Edge e1;
    e1.id = 1;
    e1.from = 1;
    e1.to = 2;
    e1.lanes_forward = 1;
    e1.polyline = {{100,0}, {200,0}};
    e1.length = polyline_length(e1.polyline);

    net.edges = {e0, e1};
    net.turn_rules.push_back({0, 1, 1, 0});

    Car c;
    c.current_edge = 0;
    c.current_lane = 0;
    c.route = {0, 1};
    c.route_index = 0;

    REQUIRE(compute_intended_lane(net, c) == 1);
}

TEST_CASE("intended lane without rule defaults to 0", "[routing]") {
    Network net;
    net.nodes = {
        {0, {0, 0}, NodeKind::Source},
        {1, {100, 0}, NodeKind::Sink}
    };
    Edge e;
    e.id = 0;
    e.to = 1;
    e.lanes_forward = 2;
    e.polyline = {{0,0}, {100,0}};
    e.length = polyline_length(e.polyline);
    net.edges = {e};

    Car c;
    c.current_edge = 0;
    c.current_lane = 1;
    c.route = {0};
    c.route_index = 0;
    REQUIRE(compute_intended_lane(net, c) == c.current_lane);

}
