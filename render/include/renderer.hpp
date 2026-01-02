#pragma once

#include <vector>
#include <cstdint>

#include "camera.hpp"
#include "color.hpp"
#include "hittable.hpp"
#include "../third_party/pcg_random_helper.hpp"

class renderer {
public:
    renderer(const camera& cam, const hittable& world);

    std::vector<color> render_tile(
        int x0, int y0,
        int tile_width, int tile_height,
        int samples_per_pixel,
        int max_depth,
        uint64_t seed
    ) const;

private:
    color ray_color(const ray& r, int depth, pcg32& rng) const;
    void print_progress(int current_scanline, int total_scanlines) const;
    const camera& cam;
    const hittable& world;
};
