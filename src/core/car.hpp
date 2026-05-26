#pragma once
#include "core/types.hpp"
#include <vector>
namespace sim {
struct Car {
    CarId id = 0;
    ProfileId profile_id = 0;

    EdgeId current_edge = 0;
    LaneIdx current_lane = 0;
    Meters offset = 0.0;

    MetersPerSec speed = 0.0;
    double accel = 0.0;

    NodeId target_exit = 0;
    std::vector<EdgeId> route;
    size_t route_index = 0;
    LaneIdx intended_lane = 0;

    TickT spawn_tick = 0;
    Meters trip_distance = 0.0;

    uint32_t lane_changes = 0;
    uint32_t violations = 0;
};
}