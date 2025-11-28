#ifndef RAYSPHERE_H
#define RAYSPHERE_H

#include "Hittable.h"

class RaySphere : public Hittable {
    public:
        RaySphere(float radius, std::shared_ptr<RayMaterial> material) : _radius(std::fmax(0.0f, radius)), _material(material) {}

        bool hit(const Ray& ray, Interval t, HitRecord& rec) const override {
            glm::vec3 o = glm::vec3(_modelMatrixI * glm::vec4(ray.origin(), 1.0f));
            glm::vec3 d = glm::vec3(_modelMatrixI * glm::vec4(ray.direction(), 0.0f));

            auto a = glm::dot(d, d);
            auto b = glm::dot(d, o);
            auto c = glm::dot(o, o) - _radius * _radius;
            auto discriminant = b * b - a * c;
            
            if (discriminant < 0)
                return false;

            auto sqrtd = std::sqrt(discriminant);

            auto root = (-b - sqrtd) / a;
            if (!t.surrounds(root)) {
                root = (-b + sqrtd) / a;
                if (!t.surrounds(root))
                    return false;
            }

            rec.t = root;
            glm::vec3 localHitPoint = o + rec.t * d;
            glm::vec3 localNormal = localHitPoint;
            rec.point = glm::vec3(_modelMatrix * glm::vec4(localHitPoint, 1.0f));
            rec.normal = glm::normalize(glm::vec3(_modelMatrixIT * glm::vec4(localNormal, 0.0f)));
            rec.setFaceNormal(ray, rec.normal);
            rec.material = _material;

            return true;
        }
    private:
        float _radius;
        std::shared_ptr<RayMaterial> _material;
};

#endif