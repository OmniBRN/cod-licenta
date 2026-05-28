#pragma once
#include "core/types.hpp"
#include <cstdint>
#include <vector>
namespace sim {

enum class NodeKind {Junction, Source, Sink};

struct Node {
    NodeId id = 0;
    Vec2 pos = {};
    NodeKind kind = NodeKind::Junction;
};

struct Edge{
    EdgeId id = 0;
    NodeId from = 0;
    NodeId to = 0;
    uint8_t lanes_forward = 1;
    std::vector<Vec2> polyline;
    Meters length = 0;
    MetersPerSec speed_limit = 50.0 * (10.0 / 36.0);
};

struct TurnRule{
    EdgeId from_edge;
    LaneIdx from_lane;
    EdgeId to_edge;
    LaneIdx to_lane;
};
    
class Network {
public:

    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<TurnRule> turn_rules;

    std::vector<std::vector<EdgeId>> out_edges;

    Vec2 point_at(EdgeId e, LaneIdx lane, Meters offset) const;
    Vec2 direction_at(EdgeId e, LaneIdx lane, Meters offset) const;

    Meters edge_length(EdgeId e) const { return edges[e].length; }

    std::vector<TurnRule> turns_from(EdgeId e, LaneIdx lane) const;

};

Meters polyline_length(const std::vector<Vec2>& pts);


}