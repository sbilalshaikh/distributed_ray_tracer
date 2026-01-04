#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "camera.hpp"
#include "color.hpp"
#include "hittable_list.hpp"
#include "math_utils.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "scene_parser.hpp"
#include "renderer.hpp"
#include "bvh.hpp"

namespace {

// Computes the bounding box of the "interesting" part of the scene, ignoring huge objects.
aabb compute_scene_bounds(const hittable_list& world) {
    aabb total_bounds;
    bool first_box = true;

    for (const auto& object : world.objects) {
        aabb object_box = object->bounding_box();
        // Heuristic: Ignore objects that are excessively large, like ground planes.
        if ((object_box.max() - object_box.min()).length() > 1000.0) {
            continue;
        }

        if (first_box) {
            total_bounds = object_box;
            first_box = false;
        } else {
            total_bounds = aabb(total_bounds, object_box);
        }
    }
    return total_bounds;
}

bool apply_framed_camera(scene& sc, const hittable_list& world, double aspect_ratio) {
    aabb bounds = compute_scene_bounds(world);
    
    // need a valid aabb to frame scene
    if (bounds.min().x() == std::numeric_limits<double>::infinity() ||
        bounds.max().x() == -std::numeric_limits<double>::infinity()) {
        return false;
    }

    auto center = point3(
        0.5 * (bounds.min().x() + bounds.max().x()),
        0.5 * (bounds.min().y() + bounds.max().y()),
        0.5 * (bounds.min().z() + bounds.max().z())
    );
    vec3 half_extent = 0.5 * (bounds.max() - bounds.min());

    double target_vfov = 50.0;
    double vfov_rad = degrees_to_radians(target_vfov);
    
    double distance_vertical = half_extent.y() / std::tan(vfov_rad / 2.0);
    double hfov_rad = 2.0 * atan(tan(vfov_rad / 2.0) * aspect_ratio);
    double distance_horizontal = half_extent.x() / std::tan(hfov_rad / 2.0);
    
    double distance = std::max({distance_vertical, distance_horizontal, 0.5});
    distance *= 1.5; // padding
    
    sc.camera.position = center + vec3(0, half_extent.y() * 0.1, distance + half_extent.z());
    sc.camera.look_at = center;
    sc.camera.up = vec3(0, 1, 0);
    sc.camera.vfov = target_vfov;

    return true;
}
} // anonymous namespace


int main(int argc, char* argv[]) {
    scene current_scene;
    bool frame_scene = false; 
    std::string scene_path;
    size_t image_width = 1200;
    int samples_per_pixel = 500;
    int max_depth = 50;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--frame-scene" || arg == "--frame-camera") {
            frame_scene = true;
        } else if (arg == "--width") {
            if (i + 1 < argc) {
                image_width = std::stoi(argv[++i]);
            }
        } else if (arg == "--samples") {
            if (i + 1 < argc) {
                samples_per_pixel = std::stoi(argv[++i]);
            }
        } else if (arg == "--depth") {
            if (i + 1 < argc) {
                max_depth = std::stoi(argv[++i]);
            }
        }
        else if (scene_path.empty()) {
            scene_path = arg;
        } else {
            std::cerr << "Warning: ignoring unknown argument '" << arg << "'\n";
        }
    }

    if (!scene_path.empty()) {
        current_scene = parse_scene(scene_path);
        std::clog << "scene parsed \n";
    } else {
        // Default scene
        auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
        auto material_center = std::make_shared<lambertian>(color(0.1, 0.2, 0.5));
        auto material_left   = std::make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
        auto material_right  = std::make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

        current_scene.world.add(std::make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
        current_scene.world.add(std::make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
        current_scene.world.add(std::make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
        current_scene.world.add(std::make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));
    }
    size_t image_height = static_cast<size_t>(image_width / (16.0/9.0));

    
    std::cerr << "Parsed world size = " << current_scene.world.objects.size() << "\n";
    
    // Note: BVH construction now happens *after* potential `frame_scene` logic
    // if we decide to base framing on the pre-BVH object list.
    if (frame_scene) {
        double aspect_ratio = double(image_width) / double(image_height);
        // We pass the original world list to compute bounds, ignoring large objects
        if (apply_framed_camera(current_scene, current_scene.world, aspect_ratio)) {
            std::cerr << "Auto camera position: "
                      << current_scene.camera.position.x() << ", "
                      << current_scene.camera.position.y() << ", "
                      << current_scene.camera.position.z()
                      << "\n";
        } else {
            std::cerr << "Warning: could not frame camera; using scene camera instead.\n";
        }
    }

    std::clog << "Constructing BVH...\n";
    hittable_list world_bvh_list;
    world_bvh_list.add(std::make_shared<bvh_node>(current_scene.world));
    std::clog << "BVH constructed.\n";

    camera cam(
        current_scene.camera.position,
        current_scene.camera.look_at,
        current_scene.camera.up,
        current_scene.camera.vfov,
        double(image_width) / image_height,
        (int)image_width,
        (int)image_height
    );
    renderer rend(cam, world_bvh_list);

    std::vector<color> out_pixels =
        rend.render_tile(
            0, 0,
            (int)image_width,
            (int)image_height,
            samples_per_pixel,
            max_depth,
            0
        );

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for (size_t j = 0; j < image_height; ++j) {
        for (size_t i = 0; i < image_width; ++i) {
            write_color(std::cout, out_pixels[j * image_width + i]);
        }
    }
}
