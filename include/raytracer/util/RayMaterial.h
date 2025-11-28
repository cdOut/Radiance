#ifndef RAYMATERIAL_H
#define RAYMATERIAL_H

#include "../hittable/Hittable.h"

class RayMaterial {
    public:
        virtual ~RayMaterial() = default;

        virtual bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const {
            return false;
        }

        virtual Color albedo() const { return Color(1.0f); }
};

class PBR : public RayMaterial {
    public:
        PBR(const Color& albedo, const float metallic, const float roughness) : _albedo(albedo), _metallic(metallic), _roughness(roughness) {}

        bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const override {
            glm::vec3 reflected = reflect(inRay.direction(), rec.normal);
            scatteredRay = Ray(rec.point, reflected);
            attenuation = _albedo;
            return true;
        }

        Color albedo() const override { return _albedo; }
    private:
        Color _albedo;
        float _metallic;
        float _roughness;
};

#endif