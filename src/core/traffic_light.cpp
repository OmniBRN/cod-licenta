#include "core/traffic_light.hpp"

namespace sim {

void TrafficLight::advance() {
    ++phase_ticks;
    TickT dur = (is_green()) ? green_duration
              : (is_yellow()) ? yellow_duration
              : red_duration;
    
    if (phase_ticks >= dur) {
        phase_ticks = 0;
        if (is_green()) color = LightColor::Yellow;
        else if (is_yellow()) color = LightColor::Red;
        else color = LightColor::Green;
    }
}

}