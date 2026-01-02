#ifndef CAMERA_H
#define CAMERA_H

#include "ray.hpp"
#include "vec3.hpp"
#include "../third_party/pcg_random_helper.hpp"

class camera {
public:
    camera(
        const point3& position,
        const point3& look_at,
        const vec3& up,
        double vfov_degrees,
        double aspect_ratio,
        int image_width,
        int image_height
    );

    ray get_ray(int i, int j, pcg32& rng) const;

private:
    vec3 pixel_sample_square(pcg32& rng) const;

    // configuration (scene-level)
    point3 position;
    point3 look_at;
    vec3 up;
    double vfov;
    double aspect_ratio;
    int image_width;
    int image_height;

    // derived (computed once)
    vec3 u, v, w;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    point3 pixel00_loc;
};
#endif
