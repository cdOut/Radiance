#ifndef HITTABLE_H
#define HITTABLE_H

#include "RaytracerUtils.h"

class RayMaterial;

class HitRecord {
    public:
        glm::vec3 point;
        glm::vec3 normal;
        float t;

        bool frontFace;

        std::shared_ptr<RayMaterial> material;

        void setFaceNormal(const Ray& ray, const glm::vec3& outwardNormal) {
            frontFace = glm::dot(ray.direction(), outwardNormal) < 0;
            normal = frontFace ? outwardNormal : -outwardNormal;
        }
};

class Hittable {
    public:
        virtual ~Hittable() = default;

        virtual bool hit(const Ray& ray, Interval t, HitRecord& rec) const = 0;
};

#endif