#include "sim/spawner.hpp"
#include <cassert>

namespace sim {

static size_t sample_categorical(Rng& rng, const std::vector<double>& probs) {
    double u = rng.uniform(0.0, 1.0);
    double acc = 0.0;
    for (size_t i = 0; i<probs.size(); ++i) {
        acc += probs[i];
        if (u <= acc) return i;
    }
    return probs.size() - 1;
}

std::vector<PendingSpawn> Spawner::sample_for_tick(Rng& arrivals, Rng& archetypes, Rng& destionations) {
    std::vector<PendingSpawn> out;
    for (const auto& src : m_sources) {
        double p = src.rate_per_sec * TICK_DT;
        assert(p< 0.1 && "rata prea mare pt aproximarea Bernoulli");
        if (!arrivals.bernoulli(p)) continue;
        size_t arch_idx = sample_categorical(archetypes, src.archetype_mix);
        size_t dest_idx = sample_categorical(destionations, src.destination_mix);
        out.push_back({src.node, static_cast<ProfileId>(arch_idx), src.destinations[dest_idx]});

    }
    return out;
}

}