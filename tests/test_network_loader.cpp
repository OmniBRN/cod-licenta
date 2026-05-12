#include <catch2/catch_test_macros.hpp>
#include "io/network_loader.hpp"

TEST_CASE("load cross.json", "[loader]") {
    auto net = sim::io::load_network("scenarios/networks/cross.json");
    REQUIRE(net.nodes.size() == 9);
    REQUIRE(net.edges.size() == 8);
    REQUIRE(net.turn_rules.size() == 12);
    REQUIRE(net.out_edges[0].size() == 4);
}

TEST_CASE("missing file throws", "[loader]") {
    REQUIRE_THROWS(sim::io::load_network("does/not/exists.json"));
}