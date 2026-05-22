#pragma once
#include "core/types.hpp"
#include "rng/prng.hpp"
#include <vector>

namespace sim {

struct SourceSpec {
    NodeId node;
    double rate_per_sec;
    std::vector<double> archetype_mix;
    std::vector<NodeId> destinations;
    std::vector<double> destination_mix;

};

struct PendingSpawn {
    NodeId source_node;
    ProfileId profile;
    NodeId destionation;
};

class Spawner {
public:
    explicit Spawner(std::vector<SourceSpec> sources) : m_sources(std::move(sources)) {}

    std::vector<PendingSpawn> sample_for_tick(Rng& arrivals, Rng& archetypes, Rng& destinations);



private:
    std::vector<SourceSpec> m_sources;

};
}
