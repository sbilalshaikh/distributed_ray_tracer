#ifndef CYLINDER_H
#define CYLINDER_H

#include "hittable.hpp"
#include "vec3.hpp"
#include <memory>

class cylinder : public hittable {
public:
    cylinder(const point3& p1, const point3& p2, double radius, std::shared_ptr<material> mat)
        : _p1(p1), _p2(p2), _radius(radius), _mat(mat) {}

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override;
    
    aabb bounding_box() const override;

    point3 p1() const { return _p1; }
    point3 p2() const { return _p2; }
    double radius() const { return _radius; }
    std::shared_ptr<material> get_material() const { return _mat; }

private:
    point3 _p1, _p2;
    double _radius;
    std::shared_ptr<material> _mat;
};

#endif
