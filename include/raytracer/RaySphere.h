#ifndef RAYSPHERE_H
#define RAYSPHERE_H

#include "Hittable.h"

class RaySphere : public Hittable {
    public:
        RaySphere(const glm::vec3& center, float radius, std::shared_ptr<RayMaterial> material) : _center(center), _radius(std::fmax(0.0f, radius)), _material(material) {}

        bool hit(const Ray& ray, Interval t, HitRecord& rec) const override {
            glm::vec3 oc = _center - ray.origin();
            auto a = glm::dot(ray.direction(), ray.direction());
            auto h = glm::dot(ray.direction(), oc);
            auto c = glm::dot(oc, oc) - _radius * _radius;
            auto discriminant = h * h - a * c;
            
            if (discriminant < 0)
                return false;

            auto sqrtd = std::sqrt(discriminant);

            auto root = (h - sqrtd) / a;
            if (!t.surrounds(root)) {
                root = (h + sqrtd) / a;
                if (!t.surrounds(root))
                    return false;
            }

            rec.t = root;
            rec.point = ray.at(rec.t);
            rec.normal = (rec.point - _center) / _radius;
            glm::vec3 outwardNormal = (rec.point - _center) / _radius;
            rec.setFaceNormal(ray, outwardNormal);
            rec.material = _material;

            return true;
        }
    private:
        glm::vec3 _center;
        float _radius;
        std::shared_ptr<RayMaterial> _material;
};

#endif