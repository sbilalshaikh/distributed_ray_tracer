#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.hpp"
#include <vector>
#include <memory>

class hittable_list : public hittable {
  public:
    std::vector<std::shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(std::shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(std::shared_ptr<hittable> object) {
        objects.push_back(object);
    }

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override;
    
    aabb bounding_box() const override;
};

#endif