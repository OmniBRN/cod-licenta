#pragma once
#include <cstdint>
#include <random>
namespace sim {
class Rng {
public:
    explicit Rng(uint64_t seed = 0): m_engine(seed) {}

    void seed(uint64_t s) { m_engine.seed(s); }

    double uniform(double a, double b) {
        std::uniform_real_distribution<double> d(a,b);
        return d(m_engine);
    }

    uint32_t uniform_int(uint32_t a, uint32_t b) {
        std::uniform_int_distribution<uint32_t> d(a,b);
        return d(m_engine);
    }

    bool bernoulli(double p) {
        std::bernoulli_distribution d(p);
        return d(m_engine);
    }

    double exponential(double lambda) {
        std::exponential_distribution<double> d(lambda);
        return d(m_engine);
    }

    std::mt19937_64& engine() { return m_engine; }
    
private:
    std::mt19937_64 m_engine;
};


// Derive independent sub-seeds from one master seed.
// Boost hash_combine prefix: 0x9E3779B97F4A7C15 = floor(2^64 / phi), the
// golden-ratio constant, gives good bit dispersion for small b (stream IDs).
// SplitMix64 finalizer (Steele/Lea/Flood, OOPSLA 2014): multipliers
// 0xBF58476D1CE4E5B9 and 0x94D049BB133111EB with shifts 30/27/31 were
// tuned together via search for full avalanche.
// Needed because mt19937_64 gives correlated output for adjacent seeds,
// so seed(master+k) would not produce independent streams.

inline uint64_t hash_combine(uint64_t a, uint64_t b) {
    uint64_t x = a + 0x9E3779B97F4A7C15ULL + (b << 6) + (b >> 2);
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

struct RngBank { 
    Rng arrivals;
    Rng archetypes;
    Rng destinations; 
    Rng compliance;
    Rng lane_change;

    void seed_all(uint64_t master) {
        arrivals.seed(hash_combine(master,1));
        archetypes.seed(hash_combine(master,2));
        destinations.seed(hash_combine(master,3));
        compliance.seed(hash_combine(master,4));
        lane_change.seed(hash_combine(master,5));
    }
};

}