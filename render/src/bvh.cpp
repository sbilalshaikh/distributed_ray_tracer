#include "bvh.hpp"

#include <numeric>
#include <random>
#include <algorithm>

static bool box_compare(const std::shared_ptr<hittable>& a, const std::shared_ptr<hittable>& b, int axis) {
    return a->bounding_box().min()[axis] < b->bounding_box().min()[axis];
}

static auto box_compare_lambda(int axis) {
    return [axis](const std::shared_ptr<hittable>& a, const std::shared_ptr<hittable>& b) {
        return box_compare(a, b, axis);
    };
}

// private recursive constructor
bvh_node::bvh_node(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {

    bbox = objects[start]->bounding_box();
    for (size_t i = start + 1; i < end; ++i) {
        bbox = aabb(bbox, objects[i]->bounding_box());
    }

    int axis = bbox.longest_axis();
    auto comparator = box_compare_lambda(axis);

    size_t object_count = end - start;

    if (object_count == 1) {
        left = right = objects[start];
    } else if (object_count == 2) {
        if (comparator(objects[start], objects[start+1])) {
            left = objects[start];
            right = objects[start+1];
        } else {
            left = objects[start+1];
            right = objects[start];
        }
    } else {
        auto mid = start + object_count / 2;
        // Partially sort the objects to find the median
        std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end, comparator);
        
        left = std::make_shared<bvh_node>(objects, start, mid);
        right = std::make_shared<bvh_node>(objects, mid, end);
    }
    
    // The bounding box for this node is the union of its children's boxes.
    // This is already computed at the start, so this line is not strictly necessary,
    // but it is correct and can be a good check.
    bbox = aabb(left->bounding_box(), right->bounding_box());
}

// Constructor for deserialization
bvh_node::bvh_node(std::shared_ptr<hittable> left, std::shared_ptr<hittable> right, aabb bbox)
    : left(left), right(right), bbox(bbox) {}

// public constructor 
bvh_node::bvh_node(const hittable_list& list) {
    auto objects = list.objects;
    *this = bvh_node(objects, 0, objects.size());
}


bool bvh_node::hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const {
    if (!bbox.hit(r, {ray_tmin, ray_tmax})) {
        return false;
    }

    bool hit_left = left->hit(r, ray_tmin, ray_tmax, rec);
    bool hit_right = right->hit(r, ray_tmin, hit_left ? rec.t : ray_tmax, rec);

    return hit_left || hit_right;
}

aabb bvh_node::bounding_box() const {
    return bbox;
}
