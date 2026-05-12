#pragma once
#include "core/network.hpp"
#include <filesystem>
namespace sim::io {

Network load_network(const std::filesystem::path& p);

}
