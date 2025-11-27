#ifndef RAYMATERIAL_H
#define RAYMATERIAL_H

#include "Hittable.h"

class RayMaterial {
    public:
        virtual ~RayMaterial() = default;

        virtual bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const {
            return false;
        }

        virtual Color albedo() const { return Color(1.0f); }
};

class Lambertian : public RayMaterial {
    public:
        Lambertian(const Color& albedo) : _albedo(albedo) {}

        bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const override {
            auto scatterDirection = rec.normal + randomUnitVector();

            if (isVectorNearZero(scatterDirection))
                scatterDirection = rec.normal;

            scatteredRay = Ray(rec.point, scatterDirection);
            attenuation = _albedo;
            return true;
        }

        Color albedo() const override { return _albedo; }
    private:
        Color _albedo;
};

class Metal : public RayMaterial {
    public:
        Metal(const Color& albedo) : _albedo(albedo) {}


        bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const override {
            glm::vec3 reflected = reflect(inRay.direction(), rec.normal);
            scatteredRay = Ray(rec.point, reflected);
            attenuation = _albedo;
            return true;
        }

        Color albedo() const override { return _albedo; }
    private:
        Color _albedo;
};

#endif