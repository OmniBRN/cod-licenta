#pragma once
#include "core/types.hpp"

namespace sim {

enum class LightColor {Green, Yellow, Red};

struct TrafficLight{
    LightColor color = LightColor::Red;
    TickT phase_ticks = 0;
    TickT green_duration = 300;
    TickT yellow_duration = 30;
    TickT red_duration = 300;

    void advance();

    bool is_red() const { return color == LightColor::Red;}
    bool is_green() const { return color == LightColor::Green;}
    bool is_yellow() const { return color == LightColor::Yellow;}

};

}