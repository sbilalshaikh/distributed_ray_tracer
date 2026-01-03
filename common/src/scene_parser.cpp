#include "scene_parser.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include "color.hpp"
#include "material.hpp"
#include "scene.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"

namespace {
std::shared_ptr<material> find_material(
    const std::map<std::string, std::shared_ptr<material>>& materials,
    const std::string& name
) {
    auto it = materials.find(name);
    if (it == materials.end() || !it->second) {
        std::cerr << "Warning: material '" << name << "' is not defined.\n";
        return nullptr;
    }
    return it->second;
}
}

scene parse_scene(const std::string& filename) {
    scene sc;
    std::map<std::string, std::shared_ptr<material>> materials;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open scene file " << filename << std::endl;
        return sc;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "material") {
            std::string name;
            std::string mat_type;
            if (!(ss >> name >> mat_type)) {
                std::cerr << "Warning: malformed material definition, skipping line: " << line << "\n";
                continue;
            }

            if (mat_type == "lambertian") {
                double r, g, b;
                if (!(ss >> r >> g >> b)) {
                    std::cerr << "Warning: invalid lambertian material '" << name << "', skipping.\n";
                    continue;
                }
                materials[name] = std::make_shared<lambertian>(color(r, g, b));
            } else if (mat_type == "metal") {
                double r, g, b, fuzz;
                if (!(ss >> r >> g >> b >> fuzz)) {
                    std::cerr << "Warning: invalid metal material '" << name << "', skipping.\n";
                    continue;
                }
                materials[name] = std::make_shared<metal>(color(r, g, b), fuzz);
            } else if (mat_type == "dielectric") {
                double ir;
                if (!(ss >> ir)) {
                    std::cerr << "Warning: invalid dielectric material '" << name << "', skipping.\n";
                    continue;
                }
                materials[name] = std::make_shared<dielectric>(ir);
            } else if (mat_type == "diffuse_light") {
                double r, g, b;
                if (!(ss >> r >> g >> b)) {
                    std::cerr << "Warning: invalid diffuse_light material '" << name << "', skipping.\n";
                    continue;
                }
                materials[name] = std::make_shared<diffuse_light>(color(r, g, b));
            } else {
                std::cerr << "Warning: unknown material type '" << mat_type << "' for material '" << name << "'.\n";
            }
        } else if (type == "sphere") {
            double cx, cy, cz, radius;
            std::string mat_name;
            if (!(ss >> cx >> cy >> cz >> radius >> mat_name)) {
                std::cerr << "Warning: malformed sphere definition, skipping line: " << line << "\n";
                continue;
            }
            auto mat_ptr = find_material(materials, mat_name);
            if (!mat_ptr) {
                continue;
            }
            sc.world.add(std::make_shared<sphere>(point3(cx, cy, cz), radius, mat_ptr));
        } else if (type == "cylinder") {
            double p1x, p1y, p1z, p2x, p2y, p2z, radius;
            std::string mat_name;
            if (!(ss >> p1x >> p1y >> p1z >> p2x >> p2y >> p2z >> radius >> mat_name)) {
                std::cerr << "Warning: malformed cylinder definition, skipping line: " << line << "\n";
                continue;
            }
            auto mat_ptr = find_material(materials, mat_name);
            if (!mat_ptr) {
                continue;
            }
            sc.world.add(std::make_shared<cylinder>(point3(p1x, p1y, p1z), point3(p2x, p2y, p2z), radius, mat_ptr));
        } else if (type == "camera") {
            while (std::getline(file, line)) {
                std::stringstream cs(line);
                std::string key;
                cs >> key;

                if (key == "end") break;

                if (key == "position") {
                    double x, y, z;
                    if (cs >> x >> y >> z) {
                        sc.camera.position = point3(x, y, z);
                    }
                } else if (key == "look_at") {
                    double x, y, z;
                    if (cs >> x >> y >> z) {
                        sc.camera.look_at = point3(x, y, z);
                    }
                } else if (key == "up") {
                    double x, y, z;
                    if (cs >> x >> y >> z) {
                        sc.camera.up = vec3(x, y, z);
                    }
                } else if (key == "vfov") {
                    cs >> sc.camera.vfov;
                }
            }
        }
    }

    return sc;
}