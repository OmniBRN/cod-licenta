#include <catch2/catch_test_macros.hpp>
#include "core/simulation.hpp"

using namespace sim;

TEST_CASE("conservation holds on no-op ticks", "[invariant]") {
    Network net;
    net.nodes = {{0, {0,0}, NodeKind::Source},
                 {1, {100,0}, NodeKind::Sink}};
    Edge e;
    e.id = 0; e.from = 0; e.to = 1; e.lanes_forward = 1;
    e.polyline = {{0,0}, {100,0}};
    e.length = polyline_length(e.polyline);
    net.edges.push_back(e);
    net.out_edges.resize(2);
    net.out_edges[0].push_back(0);

    Simulation sim(std::move(net));
    sim.seed(42);
    for (int i=0; i < 5; ++i) sim.add_car_at(0, 0, i*10.0, 0);
    REQUIRE_NOTHROW([&]{
        for (int i=0; i<1000; ++i) sim.tick();
    }());
}

TEST_CASE("rng deterministic with same seed", "[rng]") {
    sim::RngBank a, b;
    a.seed_all(123);
    b.seed_all(123);
    for (int i=0; i < 100; ++i) {
        REQUIRE(a.arrivals.uniform(0,1) == b.arrivals.uniform(0,1));
    }
    sim::RngBank c;
    c.seed_all(124);
    bool any_diff = false;
    sim::RngBank d;
    d.seed_all(123);
    for (int i=0; i < 100; ++i) {
        if (c.arrivals.uniform(0,1) != d.arrivals.uniform(0,1))
            any_diff = true;
    }
    REQUIRE(any_diff);
}