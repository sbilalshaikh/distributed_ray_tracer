#ifndef CYLINDER_H
#define CYLINDER_H

#include "hittable.hpp"
#include "vec3.hpp"
#include <memory>

class cylinder : public hittable {
public:
    cylinder(const point3& p1, const point3& p2, double radius, std::shared_ptr<material> mat)
        : p1(p1), p2(p2), radius(radius), mat(mat) {}

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override;

private:
    point3 p1, p2;
    double radius;
    std::shared_ptr<material> mat;
};

#endif
