#include "material.hpp"

// Lambertian

bool lambertian::scatter(const ray&, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const {
    auto scatter_direction = rec.normal + random_in_hemisphere(rec.normal, rng);

    // Catch degenerate scatter direction
    if (near_zero(scatter_direction))
        scatter_direction = rec.normal;

    scattered = ray(rec.p, scatter_direction);
    attenuation = albedo;
    return true;
}

// Metal

bool metal::scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const {
    vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
    scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere(rng));
    attenuation = albedo;
    return (dot(scattered.direction(), rec.normal) > 0);
}

// Dielectric

double dielectric::reflectance(double cosine, double ref_idx) {
    // Use Schlick's approximation for reflectance.
    auto r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
}

bool dielectric::scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const {
    attenuation = color(0.95, 0.95, 0.95); // -5% absorption :p
    double refraction_ratio = rec.front_face ? (1.0/ir) : ir;

    vec3 unit_direction = unit_vector(r_in.direction());
    double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
    double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

    bool cannot_refract = refraction_ratio * sin_theta > 1.0;
    vec3 direction;

    if (cannot_refract || reflectance(cos_theta, refraction_ratio) > nextDouble(rng))
        direction = reflect(unit_direction, rec.normal);
    else
        direction = refract(unit_direction, rec.normal, refraction_ratio);

    scattered = ray(rec.p, direction);
    return true;
}
