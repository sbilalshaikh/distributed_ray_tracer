#include "math_utils.hpp"
#include "camera.hpp"
#include <cmath>

camera::camera(
    const point3& position_,
    const point3& look_at_,
    const vec3& up_,
    double vfov_degrees,
    double aspect_ratio_,
    int image_width_,
    int image_height_
)
    : position(position_),
      look_at(look_at_),
      up(up_),
      vfov(vfov_degrees),
      aspect_ratio(aspect_ratio_),
      image_width(image_width_),
      image_height(image_height_) {

    auto look_vector = look_at - position;
    auto focus_dist = look_vector.length();
    if (focus_dist < 1e-6) {
        focus_dist = 1.0;
    }

    auto theta = degrees_to_radians(vfov);
    auto h = std::tan(theta / 2.0);
    auto viewport_height = 2.0 * h * focus_dist;
    auto viewport_width  = viewport_height * aspect_ratio;

    // Camera looks in the -w direction, so make w point from the look target back to the eye.
    w = unit_vector(position - look_at);
    u = unit_vector(cross(up, w));
    v = cross(w, u);

    vec3 viewport_u = viewport_width * u;
    vec3 viewport_v = viewport_height * v;

    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = -viewport_v / image_height;

    auto viewport_upper_left =
        position - focus_dist * w - viewport_u / 2.0 + viewport_v / 2.0;

    pixel00_loc =
        viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
}

ray camera::get_ray(int i, int j, pcg32& rng) const {
    auto pixel_center =
        pixel00_loc +
        i * pixel_delta_u +
        j * pixel_delta_v;

    auto pixel_sample = pixel_center + pixel_sample_square(rng);
    auto direction = pixel_sample - position;
    return ray(position, direction);
}

vec3 camera::pixel_sample_square(pcg32& rng) const {
    auto px = -0.5 + nextDouble(rng);
    auto py = -0.5 + nextDouble(rng);
    return px * pixel_delta_u + py * pixel_delta_v;
}
