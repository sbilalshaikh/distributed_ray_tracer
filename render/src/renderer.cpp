#include <iostream>
#include <iomanip> 
#include <chrono>

#include "renderer.hpp"
#include "material.hpp" 
#include "hittable.hpp"
#include "color.hpp"
#include "omp.h"
#include "ray.hpp"
#include "camera.hpp"
#include "math_utils.hpp"
#include "../third_party/pcg_random_helper.hpp"

renderer::renderer(const camera& cam_, const hittable& world_)
    : cam(cam_), world(world_) {}

std::vector<color> renderer::render_tile(
    int x0, int y0,
    int tile_width, int tile_height,
    int samples_per_pixel,
    int max_depth,
    uint64_t seed
) const {
    std::vector<color> out_pixels(
        static_cast<size_t>(tile_width) *
        static_cast<size_t>(tile_height)
    );

    #pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < tile_height; ++j) {
        pcg32 rng(seed + omp_get_thread_num());

        if (omp_get_thread_num() == 0) {
            print_progress(j, tile_height);
        }
        for (int i = 0; i < tile_width; ++i) {
            color pixel_color(0, 0, 0);

            for (int s = 0; s < samples_per_pixel; ++s) {
                // anti aliasing
                ray r = cam.get_ray(x0 + i, y0 + j, rng);
                pixel_color += ray_color(r, max_depth, rng);
            }

            out_pixels[
                static_cast<size_t>(j) * tile_width +
                static_cast<size_t>(i)
            ] = pixel_color / samples_per_pixel;
        }
    }

    return out_pixels;
}

color renderer::ray_color(const ray& r, int depth, pcg32& rng) const {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0, 0, 0);

    hit_record rec;
    // Use a slightly larger t_min to avoid self-intersection issues with floating point inaccuracies
    if (world.hit(r, 0.005, std::numeric_limits<double>::infinity(), rec)) {
        ray scattered;
        color attenuation;
        color emitted = rec.mat->emitted(rec);

        if (rec.mat->scatter(r, rec, attenuation, scattered, rng))
            return emitted + attenuation * ray_color(scattered, depth - 1, rng);

        return emitted;
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5 * (unit_direction.y() + 1.0);

    return (1.0 - t) * color(1.0, 1.0, 1.0)
         + t * color(0.5, 0.7, 1.0);
}

void renderer::print_progress(int current_scanline, int total_scanlines) const {
    if (current_scanline % 10 != 0 && current_scanline != total_scanlines - 1) return; // Update less frequently

    int bar_width = 70;
    float progress = (float)current_scanline / total_scanlines;
    int pos = bar_width * progress;

    std::clog << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::clog << "=";
        else if (i == pos) std::clog << ">";
        else std::clog << " ";
    }
    std::clog << "] " << std::fixed << std::setprecision(1) << progress * 100.0 << " %\r";
    std::clog.flush();
}
