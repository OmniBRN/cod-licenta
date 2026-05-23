#pragma once
#include "core/network.hpp"
#include "core/car.hpp"
#include <optional>

namespace sim {

std::optional<std::vector<EdgeId>> shortest_path_edges(const Network& net, NodeId from_node, NodeId to_node);

LaneIdx compute_intended_lane(const Network& net, const Car& c);

}