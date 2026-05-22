#pragma once
#include "core/simulation.hpp"
#include "core/network.hpp"
#include <utility>
namespace simtest
{
inline sim::Simulation make_two_edge_sim(double seg = 50.0)
{
    using namespace sim;
    Network net;
    net.nodes = {{0, {0, 0}, NodeKind::Source},
                 {1, {seg, 0}, NodeKind::Junction},
                 {2, {2 * seg, 0}, NodeKind::Sink}};
    Edge e0;
    e0.id = 0; e0.from = 0; e0.to = 1; e0.lanes_forward = 1;
    e0.polyline = {{0, 0}, {seg, 0}};
    e0.length = polyline_length(e0.polyline);
    Edge e1;
    e1.id = 1; e1.from = 1; e1.to = 2; e1.lanes_forward = 1;
    e1.polyline = {{seg, 0}, {2 * seg, 0}};
    e1.length = polyline_length(e1.polyline);
    net.edges = {e0, e1};
    net.out_edges.resize(3);
    net.out_edges[0] = {0};
    net.out_edges[1] = {1};
    return Simulation(std::move(net));
}
inline sim::Simulation make_line_sim(double len, uint8_t lanes = 1) {
    using namespace sim;
    Network net;
    net.nodes = { { 0, {0,0}, NodeKind::Source }, { 1, {len, 0}, NodeKind::Sink } };
    Edge e; e.id = 0; e.from = 0; e.to = 1; e.lanes_forward = lanes; 
    e.polyline = {{0,0}, {len,0}}; e.length = polyline_length(e.polyline);
    net.edges = {e};
    net.out_edges.resize(2);
    net.out_edges[0] = {0};
    return Simulation(std::move(net));
}

inline sim::Simulation make_long_lane_sim(double len) {
    return make_line_sim(len);
}
}