#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.hpp"
#include "aabb.hpp" // Added for bounding box support

#include <memory>

class material;

class hit_record {
  public:
    point3 p;
    vec3 normal;
    std::shared_ptr<material> mat;
    double t;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // normal vectors must have unit length 
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const = 0;
    
    virtual aabb bounding_box() const = 0;
};

#endif