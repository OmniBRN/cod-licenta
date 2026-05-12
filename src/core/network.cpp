#include "core/network.hpp"
#include <stdexcept>

namespace sim {

Meters polyline_length(const std::vector<Vec2>& pts){
    Meters total = 0.0;
    for(size_t i=1; i< pts.size(); ++i) {
        total += length(pts[i] - pts[i-1]);
    }
    return total;
}

Vec2 Network::point_at(EdgeId e, LaneIdx lane, Meters offset) const {
    const Edge& edge = edges[e];
    if (edge.polyline.size() < 2)
        throw std::runtime_error("Edge polyline malformed");
    Meters acc = 0.0;
    for(size_t i=1; i<edge.polyline.size(); ++i) {
        Vec2 a = edge.polyline[i-1];
        Vec2 b = edge.polyline[i];
        Meters seg = length(b - a);
        if (offset <= acc + seg || i == edge.polyline.size() - 1) {
            Meters t = (seg > 1e-9) ? (offset - acc) / seg : 0.0;
            Vec2 center = a + (b - a) * t;
            Vec2 dir = normalize(b - a);
            Vec2 right = perpendicular(dir) * -1.0;
            double lane_offset = (static_cast<double>(lane) + 0.5) * LANE_WIDTH;
            return center + right * lane_offset;
        }
        acc += seg;
    }
    return edge.polyline.back();
    
}

std::vector<TurnRule> Network::turns_from(EdgeId e, LaneIdx lane) const {
    std:: vector<TurnRule> result;
    for (const auto& r: turn_rules){
        if (r.from_edge == e && r.from_lane == lane) result.push_back(r);
    }
    return result;
}

}
