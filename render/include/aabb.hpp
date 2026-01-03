#ifndef AABB_H
#define AABB_H

#include "ray.hpp"
#include "interval.hpp"
#include "vec3.hpp"
#include <algorithm>

class aabb {
public:
    aabb() = default;
    aabb(const point3& min, const point3& max) : min_point(min), max_point(max) {}

    // Create a bounding box enclosing two other bounding boxes
    aabb(const aabb& box1, const aabb& box2) {
        min_point = point3(std::min(box1.min().x(), box2.min().x()),
                           std::min(box1.min().y(), box2.min().y()),
                           std::min(box1.min().z(), box2.min().z()));
        max_point = point3(std::max(box1.max().x(), box2.max().x()),
                           std::max(box1.max().y(), box2.max().y()),
                           std::max(box1.max().z(), box2.max().z()));
    }

    const point3& min() const { return min_point; }
    const point3& max() const { return max_point; }

    bool hit(const ray& r, interval ray_t) const {
        for (int a = 0; a < 3; ++a) {
            auto invD = 1.0f / r.direction()[a];
            auto t0 = (min_point[a] - r.origin()[a]) * invD;
            auto t1 = (max_point[a] - r.origin()[a]) * invD;

            if (invD < 0.0f) {
                std::swap(t0, t1);
            }

            if (t0 > ray_t.min) ray_t.min = t0;
            if (t1 < ray_t.max) ray_t.max = t1;

            if (ray_t.max <= ray_t.min) {
                return false;
            }
        }
        return true;
    }

    // Return the index of the longest axis
    int longest_axis() const {
        vec3 extent = max_point - min_point;
        if (extent.x() > extent.y() && extent.x() > extent.z()) {
            return 0;
        } else if (extent.y() > extent.z()) {
            return 1;
        } else {
            return 2;
        }
    }

private:
    point3 min_point;
    point3 max_point;
};

#endif
