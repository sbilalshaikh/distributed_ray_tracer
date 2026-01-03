#include "hittable_list.hpp"

bool hittable_list::hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const {
    hit_record temp_rec;
    bool hit_anything = false;
    auto closest_so_far = ray_tmax;

    for (const auto& object : objects) {
        if (object->hit(r, ray_tmin, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}

aabb hittable_list::bounding_box() const {
    if (objects.empty()) {
        return aabb();
    }

    aabb total_box = objects[0]->bounding_box();
    
    for (size_t i = 1; i < objects.size(); ++i) {
        total_box = aabb(total_box, objects[i]->bounding_box());
    }

    return total_box;
}
