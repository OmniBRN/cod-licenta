#pragma once
#include <cstdint>
#include <cmath>

namespace sim {

using NodeId = uint32_t;
using EdgeId = uint32_t;
using LaneIdx = uint8_t;
using TickT = uint64_t;
using CarId = uint32_t;
using ProfileId = uint16_t;

using Meters = double;
using MetersPerSec = double;
using Seconds = double;

constexpr Meters LANE_WIDTH = 3.5;
constexpr Seconds TICK_DT = 0.1;
constexpr Meters CAR_LENGTH = 5.0;
constexpr TickT LANE_CHANGE_TICKS = 15; 

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

inline Vec2 operator+(Vec2 a, Vec2 b) {return {a.x + b.x, a.y + b.y};}
inline Vec2 operator-(Vec2 a, Vec2 b) {return {a.x - b.x, a.y - b.y};}
inline Vec2 operator*(Vec2 a, double s) {return {a.x * s, a.y * s};}

inline double length(Vec2 v) {return std::sqrt(v.x*v.x + v.y*v.y);}

inline Vec2 normalize(Vec2 v) {
    double L = length(v);
    return (L > 1e-9) ? Vec2{v.x/L, v.y/L} : Vec2{0, 0};
}

inline Vec2 perpendicular(Vec2 v) {return {-v.y, v.x};}
}