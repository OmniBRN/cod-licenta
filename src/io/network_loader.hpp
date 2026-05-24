#pragma once
#include "core/network.hpp"
#include "core/traffic_light.hpp"
#include "sim/spawner.hpp"
#include <filesystem>
#include <vector>
#include <unordered_map>

namespace sim::io {

Network load_network(const std::filesystem::path& p);

std::vector<SourceSpec> load_sources(const std::filesystem::path& p);

std::unordered_map<EdgeId, TrafficLight> load_lights(const std::filesystem::path& p);

}

