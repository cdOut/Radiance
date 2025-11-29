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

        bool scatter(const Ray& inRay, const HitRecord& rec,
             Color& attenuation, Ray& scatteredRay) const override
{
    glm::vec3 N = glm::normalize(rec.normal);
    glm::vec3 V = glm::normalize(-inRay.direction());

    glm::vec3 L;
    float pdf = 0.0f;

    float a = _roughness * _roughness;
    glm::vec3 F0 = glm::mix(glm::vec3(0.04f), _albedo, _metallic);

    float r = randomFloat();
    bool useSpecular = (_metallic > 0.0f) && (r < _metallic);

    glm::vec3 brdf(0.0f);

    if (useSpecular) {
        // -------- SPECULAR GGX SAMPLE ----------
        glm::vec3 H = sampleGGX_VNDF(N, V, _roughness);
        L = glm::normalize(glm::reflect(-V, H));

        float NdotL = glm::dot(N, L);
        if (NdotL <= 0.0f) return false;

        float NDF = DistributionGGX(N, H, a);
        float G   = GeometrySmith(N, V, L, a);
        glm::vec3 F = fresnelSchlick(glm::max(glm::dot(H, V), 0.0f), F0);

        glm::vec3 kS = F;
        glm::vec3 kD = (glm::vec3(1.0f) - kS) * (1.0f - _metallic);

        glm::vec3 diffuse  = kD * _albedo / glm::pi<float>();

        glm::vec3 numerator = NDF * G * F;
        float denom = 4.0f * glm::max(glm::dot(N, V), 0.0f) *
                      glm::max(NdotL, 0.0f) + 0.0001f;
        glm::vec3 specular = numerator / denom;

        brdf = diffuse + specular;

        pdf = GGX_PDF_L(N, L, H, a);
        pdf = glm::max(pdf, 0.0001f);

        attenuation = brdf * NdotL / pdf;
    } else {
        // -------- PURE DIFFUSE (LAMBERT) ----------
        L = cosineSampleHemisphere(N);

        float NdotL = glm::dot(N, L);
        if (NdotL <= 0.0f) return false;

        brdf = _albedo / glm::pi<float>();  // Lambert

        pdf = NdotL / glm::pi<float>();
        pdf = glm::max(pdf, 0.0001f);

        attenuation = brdf * NdotL / pdf;   // ≈ _albedo
    }

    // clamp a bit to kill fireflies
    attenuation = glm::min(attenuation, glm::vec3(5.0f));

    float bias = 0.001f + 0.01f * _roughness;
    scatteredRay = Ray(rec.point + N * bias, glm::normalize(L));
    return true;
}


        Color albedo() const override { return _albedo; }
    private:
        Color _albedo;
        float _metallic;
        float _roughness;

        glm::vec3 cosineSampleHemisphere(const glm::vec3& N) const {
            float r1 = randomFloat();
            float r2 = randomFloat();
            
            float phi = 2.0f * glm::pi<float>() * r1;
            float cosTheta = sqrt(1.0f - r2);
            float sinTheta = sqrt(r2);

            glm::vec3 tangent = normalize(abs(N.x) > 0.1f ? glm::cross(N, glm::vec3(0,1,0)) : glm::cross(N, glm::vec3(1,0,0)));
            glm::vec3 bitangent = glm::cross(N, tangent);

            return normalize(
                tangent * cos(phi) * sinTheta +
                bitangent * sin(phi) * sinTheta +
                N * cosTheta
            );
        }

        glm::vec3 sampleGGX_VNDF(const glm::vec3& N, const glm::vec3& V, float roughness) const {
            glm::vec3 T, B;
            float a = roughness * roughness;

            if (fabsf(N.z) < 0.999f)
                T = glm::normalize(glm::cross(N, glm::vec3(0, 0, 1)));
            else
                T = glm::normalize(glm::cross(N, glm::vec3(0, 1, 0)));
            B = glm::cross(N, T);

            glm::vec3 Vh = glm::normalize(glm::vec3(
                glm::dot(V, T),
                glm::dot(V, B),
                glm::dot(V, N)
            ));

            float r1 = randomFloat();
            float r2 = randomFloat();
            float a2 = a * a;
            float phi = 2.0f * glm::pi<float>() * r1;
            float cosTheta = sqrt((1.0f - r2) / (1.0f + (a2 - 1.0f) * r2));
            float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

            glm::vec3 Ht = glm::normalize(glm::vec3(
                sinTheta * cos(phi),
                sinTheta * sin(phi),
                cosTheta
            ));

            glm::vec3 H = glm::normalize(T * Ht.x + B * Ht.y + N * Ht.z);
            return H;
        }

        float DistributionGGX(glm::vec3 N, glm::vec3 H, float a) const {
            float a2 = a * a;
            float NdotH = glm::max(glm::dot(N, H), 0.0f);
            float NdotH2 = NdotH * NdotH;
            
            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = glm::pi<float>() * denom * denom;
            
            return nom / denom;
        }

        float GGX_PDF_L(const glm::vec3& N, const glm::vec3& L, const glm::vec3& H, float a) const {
            float NdotH = glm::max(glm::dot(N, H), 0.0f);
            float HdotL = glm::max(glm::dot(H, L), 0.0f);

            float D = DistributionGGX(N, H, a);

            return (D * NdotH) / (4.0f * HdotL + 0.0001f);
        }

        float GeometrySchlickGGX(float NdotV, float k) const {
            float nom = NdotV;
            float denom = NdotV * (1.0 - k) + k;
            
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
            return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
        }
};

#endif