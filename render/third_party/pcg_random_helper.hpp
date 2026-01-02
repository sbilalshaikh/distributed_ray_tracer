#ifndef PCG_RANDOM_HELPER_H
#define PCG_RANDOM_HELPER_H

#include "pcg_random.hpp"
#include <limits>

// A helper function to generate a random double in [0, 1)
inline double nextDouble(pcg32& rng) {
    return rng() / (double)std::numeric_limits<uint32_t>::max();
}

// A helper function to generate a random double in [min, max)
inline double nextDouble(pcg32& rng, double min, double max) {
    return min + (max - min) * nextDouble(rng);
}

#endif