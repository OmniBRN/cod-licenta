#include <catch2/catch_test_macros.hpp>
#include "core/routing.hpp"
#include "core/network.hpp"

using namespace sim;

static Network make_detour_net() {
    Network net;
    net.nodes = {
        {0, {0,0}, NodeKind::Source},
        {1, {10,0}, NodeKind::Junction},
        {2, {0,50}, NodeKind::Junction},
        {3, {20,0}, NodeKind::Sink}
    };
    auto mk = [](EdgeId id, NodeId f, NodeId t, Vec2 a, Vec2 b){
        Edge e;
        e.id = id;
        e.from = f;
        e.to = t;
        e.lanes_forward = 1;
        e.polyline = {a,b};
        e.length = polyline_length(e.polyline);
        return e;
    };
    net.edges = {
        mk(0, 0, 1, {0,0}, {10,0}),
        mk(1, 1, 3, {10,0}, {20,0}),
        mk(2, 0, 2, {0,0}, {0,50}),
        mk(3, 2, 3, {0,50}, {20,0}),
    };
    net.out_edges.resize(4);
    net.out_edges[0] = {0,2};
    net.out_edges[1] = {1};
    net.out_edges[2] = {3};
    return net;

}

TEST_CASE("a-star finds direct path", "[routing]") {
    Network net = make_detour_net();
    auto path = shortest_path_edges(net, 0, 3);
    REQUIRE(path.has_value());
    REQUIRE(path->size() >= 1);
}

TEST_CASE("a-star reports no route", "[routing]") {
    Network net;
    net.nodes = 
    {
        {0, {0,0}, NodeKind::Source},
        {1, {10,0}, NodeKind::Sink}
    };
    net.out_edges.resize(2);
    auto path = shortest_path_edges(net, 0, 1);
    REQUIRE_FALSE(path.has_value());
    REQUIRE(path == std::nullopt);
}

TEST_CASE("a-star picks direct over detour", "[routing]") {
    Network net = make_detour_net();
    auto path = shortest_path_edges(net, 0, 3);
    REQUIRE(path.has_value());
    REQUIRE(*path == std::vector<EdgeId>{0,1});

    double cost = 0.0;
    for (EdgeId e: *path) cost+= net.edges[e].length;
    REQUIRE(cost == 20.0);
}