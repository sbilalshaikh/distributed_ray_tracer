#include "cylinder.hpp"
#include <algorithm>
#include <cmath>

static bool solve_quadratic(double a, double b, double c, double& t0, double& t1) {
    double discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
        return false;
    }
    double sqrt_discriminant = sqrt(discriminant);
    t0 = (-b - sqrt_discriminant) / (2*a);
    t1 = (-b + sqrt_discriminant) / (2*a);
    if (t0 > t1) {
        std::swap(t0, t1);
    }
    return true;
}

bool cylinder::hit(const ray& r, double ray_tmin, double ray_tmax, hit_record& rec) const {
    vec3 ro = r.origin();
    vec3 rd = r.direction();
    vec3 ba = p2 - p1; // Cylinder axis vector
    vec3 oc = ro - p1; // Vector from cylinder base to ray origin

    // Coefficients for quadratic equation for infinite cylinder body
    double a = dot(rd, rd) - dot(rd, unit_vector(ba)) * dot(rd, unit_vector(ba));
    double b = 2.0 * (dot(rd, oc) - dot(rd, unit_vector(ba)) * dot(oc, unit_vector(ba)));
    double c = dot(oc, oc) - dot(oc, unit_vector(ba)) * dot(oc, unit_vector(ba)) - radius*radius;

    double t0_body, t1_body;
    if (!solve_quadratic(a, b, c, t0_body, t1_body)) {
        // No intersection with infinite cylinder body
        t0_body = std::numeric_limits<double>::infinity();
        t1_body = std::numeric_limits<double>::infinity();
    }

    bool hit_body = false;
    double t_body = std::numeric_limits<double>::infinity();
    
    // Check solutions for cylinder body
    if (t0_body > ray_tmin && t0_body < ray_tmax) {
        point3 p = r.at(t0_body);
        double height = dot(p - p1, unit_vector(ba));
        if (height >= 0.0 && height <= ba.length()) {
            t_body = t0_body;
            hit_body = true;
        }
    }
    if (t1_body > ray_tmin && t1_body < ray_tmax && t1_body < t_body) {
        point3 p = r.at(t1_body);
        double height = dot(p - p1, unit_vector(ba));
        if (height >= 0.0 && height <= ba.length()) {
            t_body = t1_body;
            hit_body = true;
        }
    }

    // Check caps
    double t_cap1 = std::numeric_limits<double>::infinity();
    double t_cap2 = std::numeric_limits<double>::infinity();

    // Intersection with plane of bottom cap
    double denom1 = dot(rd, -unit_vector(ba));
    if (std::fabs(denom1) > 1e-8) { // If ray not parallel to cap plane
        t_cap1 = dot(p1 - ro, -unit_vector(ba)) / denom1;
        if (t_cap1 > ray_tmin && t_cap1 < ray_tmax) {
            point3 p_cap = r.at(t_cap1);
            if ((p_cap - p1).length_squared() > radius*radius) { // Point outside cap circle
                t_cap1 = std::numeric_limits<double>::infinity();
            }
        } else {
            t_cap1 = std::numeric_limits<double>::infinity();
        }
    }

    // Intersection with plane of top cap
    double denom2 = dot(rd, unit_vector(ba));
    if (std::fabs(denom2) > 1e-8) { // If ray not parallel to cap plane
        t_cap2 = dot(p2 - ro, unit_vector(ba)) / denom2;
        if (t_cap2 > ray_tmin && t_cap2 < ray_tmax) {
            point3 p_cap = r.at(t_cap2);
            if ((p_cap - p2).length_squared() > radius*radius) { // Point outside cap circle
                t_cap2 = std::numeric_limits<double>::infinity();
            }
        } else {
            t_cap2 = std::numeric_limits<double>::infinity();
        }
    }
    
    double t_final = std::numeric_limits<double>::infinity();
    bool hit_something = false;

    if (hit_body && t_body < t_final) {
        t_final = t_body;
        hit_something = true;
    }
    if (t_cap1 < t_final) {
        t_final = t_cap1;
        hit_something = true;
    }
    if (t_cap2 < t_final) {
        t_final = t_cap2;
        hit_something = true;
    }

    if (!hit_something) return false;

    rec.t = t_final;
    rec.p = r.at(t_final);

    vec3 outward_normal;
    if (t_final == t_body) {
        // Normal for cylinder body
        double height = dot(rec.p - p1, unit_vector(ba));
        outward_normal = unit_vector(rec.p - p1 - height * unit_vector(ba));
    } else if (t_final == t_cap1) {
        // Normal for bottom cap
        outward_normal = -unit_vector(ba);
    } else { // t_final == t_cap2
        // Normal for top cap
        outward_normal = unit_vector(ba);
    }

    rec.set_face_normal(r, outward_normal);
    rec.mat = mat;

    return true;
}

aabb cylinder::bounding_box() const {
    // A cylinder's bounding box is the union of the bounding boxes of its two end-cap spheres.
    aabb box1(p1 - vec3(radius, radius, radius), p1 + vec3(radius, radius, radius));
    aabb box2(p2 - vec3(radius, radius, radius), p2 + vec3(radius, radius, radius));
    return aabb(box1, box2);
}
