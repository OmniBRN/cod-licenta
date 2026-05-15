#include "core/behaviour.hpp"
#include <cmath>
#include <algorithm>
namespace sim {

double idm_accel(const BehaviourProfile& bp, MetersPerSec v, Meters gap, MetersPerSec dv) {
    const double a = bp.max_accel;
    const double b = bp.comfort_brake;
    const double v0 = bp.desired_speed;
    const double T = bp.reaction_time;
    const double s_min = std::max(2.0, bp.min_headway - CAR_LENGTH);
    constexpr double delta = 4.0;

    const double free_term = 1 - std::pow(v/v0, delta);

    double interact_term = 0.0;
    if (std::isfinite(gap)) {
        const double s_star = s_min + std::max(0.0, v * T * v * dv / (2.0 * std::sqrt(a * b)));
        const double s = std::max(gap, 0.1);
        interact_term = (s_star / s) * (s_star / s);
    }

    return a * (free_term - interact_term);

}

}