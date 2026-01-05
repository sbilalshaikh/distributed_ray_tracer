#include "serialization.hpp"

#include "scene.hpp" 
#include "raytracer.pb.h"
#include "hittable_list.hpp"
#include "bvh.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "material.hpp"

#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace {

// C++ -> Proto
void fill_proto_vec3(raytracer::Vec3* proto_vec, const vec3& cpp_vec) {
    proto_vec->set_x(cpp_vec.x());
    proto_vec->set_y(cpp_vec.y());
    proto_vec->set_z(cpp_vec.z());
}

// C++ -> Proto
void fill_proto_material(raytracer::Material* proto_mat, const material& cpp_mat) {
    if (const auto* p = dynamic_cast<const lambertian*>(&cpp_mat)) {
        fill_proto_vec3(proto_mat->mutable_lambertian()->mutable_albedo(), p->albedo());
    } else if (const auto* p = dynamic_cast<const metal*>(&cpp_mat)) {
        auto* proto_m = proto_mat->mutable_metal();
        fill_proto_vec3(proto_m->mutable_albedo(), p->albedo());
        proto_m->set_fuzz(p->fuzz());
    } else if (const auto* p = dynamic_cast<const dielectric*>(&cpp_mat)) {
        proto_mat->mutable_dielectric()->set_ir(p->ir());
    } else if (const auto* p = dynamic_cast<const diffuse_light*>(&cpp_mat)) {
        fill_proto_vec3(proto_mat->mutable_diffuse_light()->mutable_emit_color(), p->emitted({}));
    }
}

// recursive helper for serializing one hittable node
int32_t serialize_node(
    const std::shared_ptr<hittable>& node,
    raytracer::SceneData& scene_data,
    std::unordered_map<std::shared_ptr<hittable>, int32_t>& memo
) {
    if (!node) return -1;
    if (memo.count(node)) {
        return memo.at(node);
    }

    auto* new_node = scene_data.add_nodes();
    const int32_t current_index = scene_data.nodes_size() - 1;
    memo[node] = current_index;

    if (const auto* p = dynamic_cast<const bvh_node*>(node.get())) {
        auto* proto_bvh = new_node->mutable_bvh_node();
        fill_proto_vec3(proto_bvh->mutable_bounding_box()->mutable_min_point(), p->bounding_box().min());
        fill_proto_vec3(proto_bvh->mutable_bounding_box()->mutable_max_point(), p->bounding_box().max());
        proto_bvh->set_left_child_index(serialize_node(p->left_child(), scene_data, memo));
        proto_bvh->set_right_child_index(serialize_node(p->right_child(), scene_data, memo));
    } else if (const auto* p = dynamic_cast<const sphere*>(node.get())) {
        auto* proto_sphere = new_node->mutable_sphere();
        fill_proto_vec3(proto_sphere->mutable_center(), p->center_point());
        proto_sphere->set_radius(p->radius_value());
        fill_proto_material(proto_sphere->mutable_material(), *p->get_material());
    } else if (const auto* p = dynamic_cast<const cylinder*>(node.get())) {
        auto* proto_cyl = new_node->mutable_cylinder();
        fill_proto_vec3(proto_cyl->mutable_p1(), p->p1());
        fill_proto_vec3(proto_cyl->mutable_p2(), p->p2());
        proto_cyl->set_radius(p->radius());
        fill_proto_material(proto_cyl->mutable_material(), *p->get_material());
    }

    return current_index;
}

} 

raytracer::SceneData serialize_scene(const scene& sc) {
    raytracer::SceneData scene_data;
    std::unordered_map<std::shared_ptr<hittable>, int32_t> memo;

    // serialize camera data
    auto* proto_cam = scene_data.mutable_camera();
    fill_proto_vec3(proto_cam->mutable_position(), sc.camera.position);
    fill_proto_vec3(proto_cam->mutable_look_at(), sc.camera.look_at);
    fill_proto_vec3(proto_cam->mutable_up(), sc.camera.up);
    proto_cam->set_vfov(sc.camera.vfov);

    // expect hittable_list containing one bvh_node
    if (sc.world.objects.empty()) {
        return scene_data;
    }

    int32_t root_index = serialize_node(sc.world.objects[0], scene_data, memo);
    scene_data.set_root_node_index(root_index);

    return scene_data;
}

std::shared_ptr<hittable> deserialize_scene(const raytracer::SceneData& scene_data) {
    if (scene_data.nodes_size() == 0) {
        return nullptr;
    }

    std::vector<std::shared_ptr<hittable>> reconstructed_nodes(scene_data.nodes_size());

    // iterate backwards to ensure children are reconstructed before their parents
    for (int i = scene_data.nodes_size() - 1; i >= 0; --i) {
        const auto& node = scene_data.nodes(i);
        switch (node.node_type_case()) {
            case raytracer::SceneNode::kSphere: {
                const auto& proto_sphere = node.sphere();
                auto mat = deserialize_material(proto_sphere.material());
                reconstructed_nodes[i] = std::make_shared<sphere>(
                    proto_to_vec3(proto_sphere.center()),
                    proto_sphere.radius(),
                    mat
                );
                break;
            }
            case raytracer::SceneNode::kCylinder: {
                const auto& proto_cyl = node.cylinder();
                auto mat = deserialize_material(proto_cyl.material());
                reconstructed_nodes[i] = std::make_shared<cylinder>(
                    proto_to_vec3(proto_cyl.p1()),
                    proto_to_vec3(proto_cyl.p2()),
                    proto_cyl.radius(),
                    mat
                );
                break;
            }
            case raytracer::SceneNode::kBvhNode: {
                const auto& proto_bvh = node.bvh_node();
                auto left = reconstructed_nodes.at(proto_bvh.left_child_index());
                auto right = reconstructed_nodes.at(proto_bvh.right_child_index());
                aabb bbox(
                    proto_to_vec3(proto_bvh.bounding_box().min_point()),
                    proto_to_vec3(proto_bvh.bounding_box().max_point())
                );
                reconstructed_nodes[i] = std::make_shared<bvh_node>(left, right, bbox);
                break;
            }
            default:
                break; // shouldnt reach
        }
    }

    return reconstructed_nodes[scene_data.root_node_index()];
}
