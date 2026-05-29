#include "io/network_loader.hpp"
#include <json/json.hpp>
#include <fstream>
#include <unordered_set>
#include <stdexcept>
#include <cmath>

using nlohmann::json;

namespace sim::io {

static void require(bool cond, const std::string& msg) {
    if (!cond) throw std::runtime_error("network_loader " + msg);
}

Network load_network(const std::filesystem::path& p){
    std::ifstream f(p);
    require(f.good(), "cannot open " + p.string());

    json j;
    f >> j;
    
    Network net;

    require(j.contains("nodes") && j["nodes"].is_array(), "missing 'nodes'");
    std::unordered_set<NodeId> seen_nodes;
    for(const auto& jn: j["nodes"]) {

        Node n;
        n.id = jn.at("id").get<NodeId>();
        require(seen_nodes.insert(n.id).second, "duplicate node id " + std::to_string(n.id));
        auto pos = jn.at("pos");
        n.pos = { pos.at(0).get<double>(), pos.at(1).get<double>() };
        std::string kind = jn.value("kind", std::string{"junction"});
        if (kind == "source") n.kind = NodeKind::Source;
        else if (kind == "sink") n.kind = NodeKind::Sink;
        else n.kind = NodeKind::Junction;

        if (net.nodes.size() <= n.id) net.nodes.resize(n.id + 1);
        net.nodes[n.id] = n;

    }

    require(j.contains("edges") && j["edges"].is_array(), "missing 'edges'");
    std::unordered_set<EdgeId> seen_edges;
    for(const auto& je: j["edges"]) {

        Edge e;
        e.id = je.at("id").get<EdgeId>();
        require(seen_edges.insert(e.id).second, "duplicate edge id " + std::to_string(e.id));
        e.from = je.at("from").get<NodeId>();
        e.to = je.at("to").get<NodeId>();
        require(e.from < net.nodes.size(), "edge " + std::to_string(e.id) + " from-node missing");
        require(e.to < net.nodes.size(), "edge" + std::to_string(e.id) + " to-node missing");

        e.lanes_forward = je.at("lanes_forward").get<uint8_t>();
        require(e.lanes_forward >= 1, "edge needs >= 1 lane");

        for (const auto& pt: je.at("polyline")) {
            e.polyline.push_back({pt.at(0).get<double>(), pt.at(1).get<double>()});
        }
        require(e.polyline.size() >= 2, "polyline needs >=2 points");
        e.length = polyline_length(e.polyline);
        e.speed_limit = je.value("speed_limit_kmh", 50.0) * (10.0 / 36.0);

        if (net.edges.size() <= e.id) net.edges.resize(e.id + 1);
        net.edges[e.id] = e;
    }

    if(j.contains("turn_map")) {
        for (const auto& jt: j["turn_map"]) {
            TurnRule r;
            r.from_edge = jt.at("from_edge").get<EdgeId>();
            r.from_lane = jt.at("from_lane").get<LaneIdx>();
            r.to_edge = jt.at("to_edge").get<EdgeId>();
            r.to_lane = jt.at("to_lane").get<LaneIdx>();
            require(r.from_edge < net.edges.size() && r.to_edge < net.edges.size(), "turn refers to missing edge");
            require(r.from_lane < net.edges[r.from_edge].lanes_forward && r.to_lane < net.edges[r.to_edge].lanes_forward, "turn refers to missing lane");
            net.turn_rules.push_back(r);
        }
    }

    net.out_edges.assign(net.nodes.size(), {});
    for(const auto& e: net.edges) {
        net.out_edges[e.from].push_back(e.id);
    }

    return net;

}

std::vector<SourceSpec> load_sources(const std::filesystem::path& p) {
    std::ifstream f(p);
    require(f.good(), "cannot open " + p.string());

    json j;
    f >> j;

    require(j.contains("sources") && j["sources"].is_array(), "missing 'sources'");
    std::vector<SourceSpec> out;
    for(const auto& s: j["sources"]) {
        SourceSpec sp;
        sp.node = s.at("node").get<NodeId>();
        sp.rate_per_sec = s.at("rate_per_sec").get<double>();
        sp.archetype_mix = s.at("archetype_mix").get<std::vector<double>>();
        sp.destinations = s.at("destinations").get<std::vector<NodeId>>();
        sp.destination_mix = s.at("destination_mix").get<std::vector<double>>();

        const std::string id = "source node " + std::to_string(sp.node);
        require(sp.archetype_mix.size() == 5,
                id + ": archetype_mix must have exactly 5 entries");
        require(!sp.destinations.empty(),
                id + ": destinations must not be empty");
        require(sp.destinations.size() == sp.destination_mix.size(),
                id + ": destinations and destination_mix length mismatch");

        auto check_sum = [&](const std::vector<double>& v, const char* what) {
            double sum = 0.0;
            for (double x : v) {
                require(x >= 0.0, id + ": " + what + " has a negative entry");
                sum += x;
            }
            require(std::abs(sum - 1.0) < 1e-6,
                    id + ": " + what + " must sum to 1.0");
        };
        check_sum(sp.archetype_mix, "archetype_mix");
        check_sum(sp.destination_mix, "destination_mix");

        out.push_back(std::move(sp));
    }
    return out;
}

std::unordered_map<EdgeId, TrafficLight> load_lights(const std::filesystem::path& p) {
    std::ifstream f(p);
    require(f.good(), "cannot open " + p.string());

    json j;
    f >> j;

    require(j.contains("lights") && j["lights"].is_array(), "missing 'lights'");
    std::unordered_map<EdgeId, TrafficLight> out;
    for(const auto& jl : j["lights"]) {
        EdgeId eid = jl.at("edge").get<EdgeId>();
        TrafficLight tl;
        tl.green_duration = jl.at("green_dur").get<TickT>();
        tl.yellow_duration = jl.at("yellow_dur").get<TickT>();
        tl.red_duration = jl.at("red_dur").get<TickT>();
        std::string start_color = jl.at("start_color").get<std::string>();
        if (start_color == "green") tl.color = LightColor::Green;
        else if (start_color == "yellow") tl.color = LightColor::Yellow;
        else tl.color = LightColor::Red;
        out[eid] = tl;
    }
    return out;
}

}

