#pragma once
#include "core/network.hpp"
#include "sim/spawner.hpp"
#include <filesystem>
#include <vector>

namespace sim::io {

Network load_network(const std::filesystem::path& p);

std::vector<SourceSpec> load_sources(const std::filesystem::path& p);

}

