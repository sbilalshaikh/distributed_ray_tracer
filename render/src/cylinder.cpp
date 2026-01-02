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
    vec3 axis = p2 - p1;
    double axis_len_sq = dot(axis, axis);
    vec3 axis_norm = axis / sqrt(axis_len_sq);
    vec3 oc = r.origin() - p1;

    vec3 a_comp = r.direction() - dot(r.direction(), axis_norm) * axis_norm;
    double a = dot(a_comp, a_comp);

    vec3 b_comp = oc - dot(oc, axis_norm) * axis_norm;
    double b = 2.0 * dot(a_comp, b_comp);
    double c = dot(b_comp, b_comp) - radius*radius;

    double t0, t1;
    if (!solve_quadratic(a, b, c, t0, t1)) {
        return false;
    }

    bool hit = false;
    double closest_t = ray_tmax;

    // Check body intersections
    for (double t : {t0, t1}) {
        if (t > ray_tmin && t < closest_t) {
            point3 p = r.at(t);
            double height = dot(p - p1, axis_norm);
            if (height >= 0 && height <= sqrt(axis_len_sq)) {
                hit = true;
                closest_t = t;
                rec.t = t;
                rec.p = p;
                vec3 outward_normal = (p - p1 - height * axis_norm) / radius;
                rec.set_face_normal(r, outward_normal);
                rec.mat = mat;
            }
        }
    }

    // Check cap intersections
    // Bottom cap
    double t_cap1 = dot(p1 - r.origin(), axis_norm) / dot(r.direction(), axis_norm);
    if (t_cap1 > ray_tmin && t_cap1 < closest_t) {
        point3 p_cap = r.at(t_cap1);
        if (dot(p_cap - p1, p_cap - p1) < radius*radius) {
            hit = true;
            closest_t = t_cap1;
            rec.t = t_cap1;
            rec.p = p_cap;
            rec.set_face_normal(r, -axis_norm);
            rec.mat = mat;
        }
    }

    // Top cap
    double t_cap2 = dot(p2 - r.origin(), axis_norm) / dot(r.direction(), axis_norm);
    if (t_cap2 > ray_tmin && t_cap2 < closest_t) {
        point3 p_cap = r.at(t_cap2);
        if (dot(p_cap - p2, p_cap - p2) < radius*radius) {
            hit = true;
            closest_t = t_cap2;
            rec.t = t_cap2;
            rec.p = p_cap;
            rec.set_face_normal(r, axis_norm);
            rec.mat = mat;
        }
    }

    return hit;
}


