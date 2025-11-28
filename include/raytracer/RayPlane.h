#ifndef RAYPLANE_H
#define RAYPLANE_H

#include "Hittable.h"

class RayPlane : public Hittable {
    public:
        RayPlane(std::shared_ptr<RayMaterial> material) : _material(material) {}

        bool hit(const Ray& ray, Interval t, HitRecord& rec) const override {
            glm::vec3 o = glm::vec3(_modelMatrixI * glm::vec4(ray.origin(), 1.0f));
            glm::vec3 d = glm::vec3(_modelMatrixI * glm::vec4(ray.direction(), 0.0f));

            if (fabs(d.y) < 1e-6f)
                return false;

            float root = -o.y / d.y;
            if (!t.surrounds(root))
                return false;

            rec.t = root;
            glm::vec3 localHitPoint = o + rec.t * d;
            if (localHitPoint.x < -0.5f || localHitPoint.x > 0.5f || localHitPoint.z < -0.5f || localHitPoint.z > 0.5f)
                return false;
            glm::vec3 localNormal(0.0f, 1.0f, 0.0f);
            rec.point = glm::vec3(_modelMatrix * glm::vec4(localHitPoint, 1.0f));
            rec.normal = glm::normalize(glm::vec3(_modelMatrixIT * glm::vec4(localNormal, 0.0f)));
            rec.setFaceNormal(ray, rec.normal);
            rec.material = _material;

            return true;
        }
    private:
        std::shared_ptr<RayMaterial> _material;
};

#endif