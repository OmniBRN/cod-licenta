#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"

using namespace sim;

TEST_CASE("mandatory lane change occurs", "[lanechange]") {
    using namespace sim;
    Network net;
    net.nodes = {
        {0, {0,0}, NodeKind::Source},
        {1, {500,0}, NodeKind::Junction},
        {2, {1000,0}, NodeKind::Sink}
    };
    Edge e0; e0.id=0; e0.from=0; e0.to=1; e0.lanes_forward=2;
    e0.polyline={{0,0},{500,0}}; e0.length=polyline_length(e0.polyline);
    Edge e1; e1.id=1; e1.from=1; e1.to=2; e1.lanes_forward=2;
    e1.polyline={{500,0},{1000,0}}; e1.length=polyline_length(e1.polyline);
    net.edges={e0,e1};
    net.out_edges.resize(3);
    net.out_edges[0]={0}; net.out_edges[1]={1};
    TurnRule tr; tr.from_edge=0; tr.from_lane=0; tr.to_edge=1; tr.to_lane=0;
    net.turn_rules.push_back(tr);
    Simulation s(std::move(net));

    Car proto;
    proto.current_edge = 0;
    proto.current_lane = 1;
    proto.intended_lane = 1;
    proto.offset = 50.0;
    proto.speed = 10.0;
    proto.route = {0, 1};
    proto.route_index = 0;
    proto.profile_id = 0;
    CarId id = s.spawn_car(proto);

    for(int i=0; i<200; ++i) s.tick();

    const Car* c = nullptr;
    for(const auto& x: s.cars()) if (x.id == id) c = &x;
    REQUIRE(c != nullptr);
    REQUIRE(c->current_lane == 0);
}