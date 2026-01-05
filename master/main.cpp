#include <iostream>
#include <memory>
#include "cxxopts.hpp"
#include "master.hpp"
#include "scene.hpp"
#include "scene_parser.hpp"
#include "bvh.hpp"

int main(int argc, char** argv) {
    cxxopts::Options options("Raytracer Master", "The master node for the distributed raytracer.");
    options.add_options()
        ("s,scene", "Scene file path", cxxopts::value<std::string>())
        ("o,output", "Output image file path", cxxopts::value<std::string>()->default_value("output.ppm"))
        ("w,width", "Image width", cxxopts::value<int>()->default_value("1200"))
        ("h,height", "Image height", cxxopts::value<int>()->default_value("800"))
        ("p,port", "Port to listen on", cxxopts::value<int>()->default_value("50051"))
        ("samples", "Samples per pixel", cxxopts::value<int>()->default_value("100"))
        ("depth", "Max ray depth", cxxopts::value<int>()->default_value("50"))
        ("tile-size", "Size of render tiles", cxxopts::value<int>()->default_value("64"))
        ("help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help") || !result.count("scene")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    std::string scene_path = result["scene"].as<std::string>();
    scene current_scene = parse_scene(scene_path);
    hittable_list world_bvh;
    world_bvh.add(std::make_shared<bvh_node>(current_scene.world));
    current_scene.world = world_bvh;

    std::string address = "0.0.0.0:" + std::to_string(result["port"].as<int>());
    std::string output_path = result["output"].as<std::string>();

    try {
        RunServer(
            current_scene,
            result["width"].as<int>(),
            result["height"].as<int>(),
            result["tile-size"].as<int>(),
            result["samples"].as<int>(),
            result["depth"].as<int>(),
            address,
            output_path
        );
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}