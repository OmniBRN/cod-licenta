#pragma once
#include "core/network.hpp"
#include <optional>

namespace sim {

std::optional<std::vector<EdgeId>> shortest_path_edges(const Network& net, NodeId from_node, NodeId to_node);

}