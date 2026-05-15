#pragma once
#include "core/types.hpp"
#include <string>
namespace sim {

struct BehaviourProfile {
    std::string name = "default";
    // Temporary Values to be replaced by IDM
    MetersPerSec desired_speed = 50.0 * (10.0/36.0); // 50 km/h;
    double max_accel = 1.5;
    double comfort_brake = 2.0;
    Seconds reaction_time = 1.2;
    Meters min_headway = 8.0;

    // Behaviour values [0,1]
    double aggressiveness = 0.5;
    double traffic_law_compliance = 0.95;

};

double idm_accel(const BehaviourProfile& bp, MetersPerSec v, Meters gap, MetersPerSec dv);

inline BehaviourProfile default_profile(){ return {}; };

}