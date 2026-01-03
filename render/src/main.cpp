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
#include "scene_parser.hpp"
#include "renderer.hpp"
#include "bvh.hpp"


int main(int argc, char* argv[]) {
    scene current_scene;
    bool show_axes = false; // This option will no longer add axes, as aabb is removed.
    bool frame_scene = false; // This option will no longer frame the camera, as aabb is removed.
    std::string scene_path;
    size_t image_width = 1200;
    int samples_per_pixel = 500;
    int max_depth = 50;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--show-axes" || arg == "--axes") {
            std::cerr << "Warning: --show-axes is deprecated as AABB (voxel) support has been removed.\n";
            show_axes = true;
        } else if (arg == "--frame-scene" || arg == "--frame-camera") {
            std::cerr << "Warning: --frame-scene is deprecated as AABB (voxel) support has been removed.\n";
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

    // Removed frame_scene and show_axes logic as AABB support is removed.
    
    std::cerr << "Parsed world size = " << current_scene.world.objects.size() << "\n";
    
    std::clog << "Constructing BVH...\n";
    hittable_list world_bvh;
    world_bvh.add(std::make_shared<bvh_node>(current_scene.world));
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
    renderer rend(cam, world_bvh);

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
