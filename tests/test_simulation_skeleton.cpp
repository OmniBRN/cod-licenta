#include <catch2/catch_test_macros.hpp>
#include "core/simulation.hpp"
#include "core/network.hpp"

using namespace sim;

static Network make_trivial_net(){
    Network net;
    net.nodes = {{0, {0, 0}, NodeKind::Source}, 
                 {1, {100, 0}, NodeKind::Sink}};
    Edge e;
    e.id = 0; e.from = 0; e.to = 1;
    e.lanes_forward = 1;
    e.polyline = {{0,0}, {100, 0}};
    e.length = polyline_length(e.polyline);
    net.edges.push_back(e);
    net.out_edges.resize(2);
    net.out_edges[0].push_back(0);
    return net;
}

TEST_CASE("simulation_skeletonm", "[sim]") {
    Simulation sim(make_trivial_net());
    sim.add_car_at(0,0, 10.0, 0);
    sim.add_car_at(0,0, 30.0, 0);
    sim.add_car_at(0,0, 50.0, 0);
    for(int i=0; i < 100; ++i) sim.tick();
    REQUIRE(sim.cars().size() == 3);
    REQUIRE(sim.current_tick() == 100);
}