#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <memory>
#include "vec3.hpp" 
#include "material.hpp"
#include "raytracer.pb.h"

class scene;
class hittable;

raytracer::SceneData serialize_scene(const scene& sc);

std::shared_ptr<hittable> deserialize_scene(const raytracer::SceneData& scene_data);

inline vec3 proto_to_vec3(const raytracer::Vec3& proto_vec) {
    return vec3(proto_vec.x(), proto_vec.y(), proto_vec.z());
}

inline std::shared_ptr<material> deserialize_material(const raytracer::Material& proto_mat) {
    switch (proto_mat.material_type_case()) {
        case raytracer::Material::kLambertian:
            return std::make_shared<lambertian>(proto_to_vec3(proto_mat.lambertian().albedo()));
        case raytracer::Material::kMetal:
            return std::make_shared<metal>(proto_to_vec3(proto_mat.metal().albedo()), proto_mat.metal().fuzz());
        case raytracer::Material::kDielectric:
            return std::make_shared<dielectric>(proto_mat.dielectric().ir());
        case raytracer::Material::kDiffuseLight:
            return std::make_shared<diffuse_light>(proto_to_vec3(proto_mat.diffuse_light().emit_color()));
        default:
            return nullptr;
    }
}

vec3 proto_to_vec3(const raytracer::Vec3& proto_vec);
std::shared_ptr<material> deserialize_material(const raytracer::Material& proto_mat);


#endif 