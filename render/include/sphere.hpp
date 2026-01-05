#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.hpp"
#include "vec3.hpp"

#include <cmath>
#include <memory>

class sphere : public hittable {
  public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat)
      : center(center), radius(std::fmax(0,radius)), mat(mat) {}

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override;

    aabb bounding_box() const override;

    const point3& center_point() const { return center; }
    double radius_value() const { return radius; }
    std::shared_ptr<material> get_material() const { return mat; }

  private:
    point3 center;
    double radius;
    std::shared_ptr<material> mat;
};

#endif
