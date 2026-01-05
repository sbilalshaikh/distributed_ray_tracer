#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "cxxopts.hpp"

#include "scene_parser.hpp"
#include "renderer.hpp"
#include "bvh.hpp"
#include "camera.hpp"
#include "color.hpp"
#include "hittable_list.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "math_utils.hpp"


namespace {
// Computes the bounding box of the "interesting" part of the scene, ignoring huge objects.
aabb compute_scene_bounds(const hittable_list& world) {
    aabb total_bounds;
    bool first_box = true;

    for (const auto& object : world.objects) {
        aabb object_box = object->bounding_box();
        if ((object_box.max() - object_box.min()).length() > 1000.0) continue;

        total_bounds = first_box ? object_box : aabb(total_bounds, object_box);
        first_box = false;
    }
    return total_bounds;
}

bool apply_framed_camera(scene& sc, const hittable_list& world, double aspect_ratio) {
    aabb bounds = compute_scene_bounds(world);
    if (bounds.min().x() == std::numeric_limits<double>::infinity()) return false;

    auto center = point3(0.5 * (bounds.min().x() + bounds.max().x()), 0.5 * (bounds.min().y() + bounds.max().y()), 0.5 * (bounds.min().z() + bounds.max().z()));
    vec3 half_extent = 0.5 * (bounds.max() - bounds.min());

    double target_vfov = 50.0;
    double vfov_rad = degrees_to_radians(target_vfov);
    double hfov_rad = 2.0 * atan(tan(vfov_rad / 2.0) * aspect_ratio);
    double distance = std::max(half_extent.y() / std::tan(vfov_rad / 2.0), half_extent.x() / std::tan(hfov_rad / 2.0));
    
    sc.camera.position = center + vec3(0, half_extent.y() * 0.1, (distance * 1.5) + half_extent.z());
    sc.camera.look_at = center;
    sc.camera.up = vec3(0, 1, 0);
    sc.camera.vfov = target_vfov;

    return true;
}
} 


int main(int argc, char* argv[]) {
    cxxopts::Options options("Raytracer", "A simple standalone raytracer.");
    options.add_options()
        ("s,scene", "Scene file path", cxxopts::value<std::string>())
        ("w,width", "Image width", cxxopts::value<int>()->default_value("1200"))
        ("h,height", "Image height", cxxopts::value<int>()->default_value("800"))
        ("samples", "Samples per pixel", cxxopts::value<int>()->default_value("100"))
        ("depth", "Max ray depth", cxxopts::value<int>()->default_value("50"))
        ("f,frame-scene", "Automatically frame the scene", cxxopts::value<bool>()->default_value("false"))
        ("help", "Print usage");
    
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    scene current_scene;
    if (result.count("scene")) {
        current_scene = parse_scene(result["scene"].as<std::string>());
        std::clog << "Scene parsed." << std::endl;
    } else {
        std::clog << "No scene file provided. Creating default scene." << std::endl;
        auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
        auto material_center = std::make_shared<lambertian>(color(0.1, 0.2, 0.5));
        auto material_left   = std::make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
        auto material_right  = std::make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
        current_scene.world.add(std::make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
        current_scene.world.add(std::make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
        current_scene.world.add(std::make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
        current_scene.world.add(std::make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
    }

    int image_width = result["width"].as<int>();
    int image_height = result["height"].as<int>();
    double aspect_ratio = static_cast<double>(image_width) / image_height;

    if (result["frame-scene"].as<bool>()) {
        if (apply_framed_camera(current_scene, current_scene.world, aspect_ratio)) {
            std::clog << "Auto-framing camera." << std::endl;
        } else {
            std::cerr << "Warning: could not frame camera." << std::endl;
        }
    }

    std::clog << "Constructing BVH..." << std::endl;
    hittable_list world_bvh;
    world_bvh.add(std::make_shared<bvh_node>(current_scene.world));
    std::clog << "BVH constructed." << std::endl;

    camera cam(
        current_scene.camera.position,
        current_scene.camera.look_at,
        current_scene.camera.up,
        current_scene.camera.vfov,
        aspect_ratio,
        image_width,
        image_height
    );

    renderer rend(cam, world_bvh);
    std::vector<color> out_pixels = rend.render_tile(
        0, 0, image_width, image_height,
        result["samples"].as<int>(),
        result["depth"].as<int>(),
        0
    );

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for (const auto& pixel : out_pixels) {
        write_color(std::cout, pixel);
    }
    
    return 0;
}