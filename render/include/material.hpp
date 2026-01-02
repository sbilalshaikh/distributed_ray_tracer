#ifndef MATERIAL_H
#define MATERIAL_H

#include "ray.hpp"
#include "hittable.hpp"
#include "color.hpp"

class material {
  public:
    virtual ~material() = default;

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng
    ) const = 0;

    virtual color emitted(const hit_record&) const { return color(0, 0, 0); }
};

class lambertian : public material {
  public:
    lambertian(const color& albedo) : albedo(albedo) {}

    bool scatter(const ray&, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const override;

  private:
    color albedo;
};

class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const override;

  private:
    color albedo;
    double fuzz;
};

class dielectric : public material {
  public:
    dielectric(double refractive_index) : ir(refractive_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const override;

  private:
    double ir; // Index of Refraction

    static double reflectance(double cosine, double ref_idx);
};

class diffuse_light : public material {
  public:
    diffuse_light(const color& emit_color) : emit(emit_color) {}

    bool scatter(const ray&, const hit_record&, color&, ray&, pcg32&) const override {
        return false;
    }

    color emitted(const hit_record&) const override { return emit; }

  private:
    color emit;
};

#endif
