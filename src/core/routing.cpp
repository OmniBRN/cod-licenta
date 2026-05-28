#include "core/routing.hpp"
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>

namespace sim {

static double euclid(Vec2 a, Vec2 b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy* dy);
}

std::optional<std::vector<EdgeId>> shortest_path_edges (const Network& net, NodeId from_node, NodeId to_node) {
    const size_t N = net.nodes.size();
    constexpr double INF = std::numeric_limits<double>::infinity();

    std::vector<double> g_score(N, INF);
    std::vector<EdgeId> prev_edge(N, 0);
    std::vector<NodeId> prev_node(N, 0);
    std::vector<uint8_t> closed(N, 0);

    const Vec2 goal_pos = net.nodes[to_node].pos;
    auto h = [&](NodeId u) {return euclid(net.nodes[u].pos, goal_pos);};

    using queue_item = std::pair<double, NodeId>;
    std::priority_queue<queue_item, std::vector<queue_item>, std::greater<queue_item>> open;

    g_score[from_node] = 0.0;

    open.push({h(from_node), from_node});

    while(!open.empty()) {
        auto [f,u] = open.top();
        open.pop();
        if(closed[u]) continue;
        closed[u] = 1;
        if (u==to_node) break;
        
        for(EdgeId e : net.out_edges[u]) {
            const Edge& edge = net.edges[e];
            NodeId v = edge.to;
            if (closed[v]) continue;
            double tentative_g = g_score[u] + edge.length;
            if (tentative_g < g_score[v]) {
                g_score[v] = tentative_g;
                prev_edge[v] = e;
                prev_node[v] = u;
                open.push({tentative_g + h(v), v});
            }
        }
    }

    if (g_score[to_node] == INF) return std::nullopt;

    std::vector<EdgeId> path;
    for (NodeId v = to_node; v != from_node; v = prev_node[v]) {
        path.push_back(prev_edge[v]);
    }

    std::reverse(path.begin(), path.end());
    return path;

}

LaneIdx compute_intended_lane(const Network& net, const Car& c) {
    if (c.route.empty() || c.route_index + 1 >= c.route.size())
        return c.current_lane;

    EdgeId cur = c.route[c.route_index];
    EdgeId next = c.route[c.route_index + 1];

    bool any_rule = false;
    LaneIdx best = c.current_lane;
    uint8_t best_dist = 255;
    for (const auto& r: net.turn_rules) {
        if (r.from_edge == cur && r.to_edge == next) {
            any_rule = true;
            if (r.from_lane == c.current_lane)
                return c.current_lane;
            uint8_t dist = (r.from_lane > c.current_lane)
                         ? (r.from_lane - c.current_lane)
                         : (c.current_lane - r.from_lane);
            if (dist < best_dist) { best_dist = dist; best = r.from_lane; }
        }
    }
    return any_rule ? best : c.current_lane;
}
}