#ifndef RAYMATERIAL_H
#define RAYMATERIAL_H

#include "../hittable/Hittable.h"
#include "../light/RayLight.h"
#include <glm/ext/scalar_constants.hpp>

class RayMaterial {
    public:
        virtual ~RayMaterial() = default;

        virtual bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const {
            return false;
        }

        virtual Color shade(const Ray& inRay, const HitRecord& rec, const glm::vec3& lightDir, const RayLight& light) const {
            float nDotL = glm::max(glm::dot(rec.normal, glm::normalize(lightDir)), 0.0f);
            return Color(1.0f) * light.intensityAt(rec.point) * nDotL / glm::pi<float>();
        }
};

class PBR : public RayMaterial {
    public:
        PBR(const Color& albedo, float metallic, float roughness)
            : _albedo(albedo),
              _metallic(glm::clamp(metallic, 0.0f, 1.0f)),
              _roughness(glm::clamp(roughness, 0.05f, 1.0f)) {}

        Color shade(const Ray& inRay, const HitRecord& rec, const glm::vec3& lightDir, const RayLight& light) const override {
            glm::vec3 N = glm::normalize(rec.normal);
            glm::vec3 V = glm::normalize(-inRay.direction());
            glm::vec3 L = glm::normalize(lightDir);
            glm::vec3 H = glm::normalize(V + L);

            float NdotL = glm::max(glm::dot(N, L), 0.0f);
            float NdotV = glm::max(glm::dot(N, V), 0.0f);
            if (NdotL <= 0.0f || NdotV <= 0.0f) return Color(0.0f);

            Color F0 = glm::mix(Color(0.04f), _albedo, _metallic);

            float NDF = DistributionGGX(N, H, _roughness);
            float G = GeometrySmith(N, V, L, _roughness);
            glm::vec3 F = fresnelSchlick(glm::max(glm::dot(H, V), 0.0f), F0);

            glm::vec3 kS = F;
            glm::vec3 kD = (glm::vec3(1.0f) - kS) * (1.0f - _metallic);

            glm::vec3 numerator = NDF * G * F;
            float denominator = 4.0f * glm::max(glm::dot(N, V), 0.0f) * glm::max(glm::dot(N, L), 0.0f) + 1e-4f;
            glm::vec3 specular = numerator / denominator;

            Color radiance = light.intensityAt(rec.point);

            return (kD * _albedo / glm::pi<float>() + specular) * radiance * NdotL;
        }

        bool scatter(const Ray& inRay, const HitRecord& rec, Color& attenuation, Ray& scatteredRay) const override {
            glm::vec3 N = rec.normal;
            glm::vec3 V = glm::normalize(-inRay.direction());

            Color F0 = glm::mix(Color(0.04f), _albedo, _metallic);
            glm::vec3 F = fresnelSchlick(glm::max(glm::dot(N, V), 0.0f), F0);

            float specularChance = glm::max(F.r, glm::max(F.g, F.b));

            glm::vec3 dir;
            if (randomFloat() < specularChance) {
                glm::vec3 R = reflect(glm::normalize(inRay.direction()), N);
                dir = glm::normalize(R + _roughness * _roughness * randomOnHemisphere(N));
                attenuation = F / specularChance;
            } else {
                dir = randomCosineHemisphere(N);
                glm::vec3 kD = (glm::vec3(1.0f) - F) * (1.0f - _metallic);
                attenuation = kD * _albedo / (1.0f - specularChance);
            }

            if (glm::dot(dir, N) <= 0.0f) dir = N;
            scatteredRay = Ray(rec.point + N * 0.001f, dir);
            return true;
        }
    private:
        Color _albedo;
        float _metallic;
        float _roughness;

        float DistributionGGX(glm::vec3 N, glm::vec3 H, float a) const {
            float a2 = a * a;
            float NdotH = glm::max(glm::dot(N, H), 0.0f);
            float NdotH2 = NdotH * NdotH;

            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
            denom = glm::pi<float>() * denom * denom;

            return nom / denom;
        }

        float GeometrySchlickGGX(float NdotV, float k) const {
            float nom = NdotV;
            float denom = NdotV * (1.0f - k) + k;

            return nom / denom;
        }

        float GeometrySmith(glm::vec3 N, glm::vec3 V, glm::vec3 L, float k) const {
            float NdotV = glm::max(glm::dot(N, V), 0.0f);
            float NdotL = glm::max(glm::dot(N, L), 0.0f);
            float ggx1 = GeometrySchlickGGX(NdotV, k);
            float ggx2 = GeometrySchlickGGX(NdotL, k);

            return ggx1 * ggx2;
        }

        glm::vec3 fresnelSchlick(float cosTheta, glm::vec3 F0) const {
            return F0 + (1.0f - F0) * powf(1.0f - cosTheta, 5.0f);
        }
};

#endif
