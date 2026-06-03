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

inline BehaviourProfile ideal_profile() {
    BehaviourProfile p;
    p.name = "ideal";
    p.desired_speed = 50 * (10.0/36.0);
    p.max_accel = 1.5;
    p.comfort_brake = 2.0;
    p.reaction_time = 1.2;
    p.min_headway = 10.0;
    p.aggressiveness = 0.0;
    p.traffic_law_compliance = 1.0;
    return p;
}

inline BehaviourProfile cautious_profile() {
    BehaviourProfile p;
    p.name = "cautious";
    p.desired_speed = 40 * (10.0/36.0);
    p.max_accel = 1.0;
    p.comfort_brake = 1.5;
    p.reaction_time = 1.6;
    p.min_headway = 14.0;
    p.aggressiveness = 0.1;
    p.traffic_law_compliance = 0.99;
    return p;
}

inline BehaviourProfile normal_profile() { return default_profile(); }

inline BehaviourProfile aggresive_profile() {
    BehaviourProfile p;
    p.name = "aggresive";
    p.desired_speed = 70 * (10.0/36.0);
    p.max_accel = 2.5;
    p.comfort_brake = 3.0;
    p.reaction_time = 0.8;
    p.min_headway = 4.0;
    p.aggressiveness = 0.9;
    p.traffic_law_compliance = 0.7;
    return p;
}

inline BehaviourProfile opportunist_profile() {
    BehaviourProfile p;
    p.name = "opportunist";
    p.desired_speed = 50 * (10.0/36.0);
    p.max_accel = 1.5;
    p.comfort_brake = 2.0;
    p.reaction_time = 1.2;
    p.min_headway = 8.0;
    p.aggressiveness = 0.85;
    p.traffic_law_compliance = 0.95;
    return p;
}

}