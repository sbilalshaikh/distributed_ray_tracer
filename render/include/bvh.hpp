#ifndef BVH_H
#define BVH_H

#include "hittable.hpp"
#include "hittable_list.hpp"
#include <vector>
#include <memory>
#include <algorithm>

class bvh_node : public hittable {
public:
    bvh_node(const hittable_list& list);
    bvh_node(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end);

    bool hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const override;
    aabb bounding_box() const override;

private:
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    aabb bbox;
};

#endif
