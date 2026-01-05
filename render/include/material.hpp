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
    lambertian(const color& albedo) : _albedo(albedo) {}
    bool scatter(const ray&, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const override;
    color albedo() const { return _albedo; }
  private:
    color _albedo;
};

class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : _albedo(albedo), _fuzz(fuzz < 1 ? fuzz : 1) {}
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const override;
    color albedo() const { return _albedo; }
    double fuzz() const { return _fuzz; }
  private:
    color _albedo;
    double _fuzz;
};

class dielectric : public material {
  public:
    dielectric(double refractive_index) : _ir(refractive_index) {}
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, pcg32& rng) const override;
    double ir() const { return _ir; }
  private:
    double _ir; // Index of Refraction
    static double reflectance(double cosine, double ref_idx);
};

class diffuse_light : public material {
  public:
    diffuse_light(const color& emit_color) : _emit(emit_color) {}
    bool scatter(const ray&, const hit_record&, color&, ray&, pcg32&) const override {
        return false;
    }
    color emitted(const hit_record&) const override { return _emit; }
  private:
    color _emit;
};

#endif
