#ifndef HITTABLELIST_H
#define HITTABLELIST_H

#include "Hittable.h"
#include "../util/RaytracerUtils.h"
#include <vector>

class HittableList : public Hittable {
    public:
        std::vector<std::shared_ptr<Hittable>> objects;

        HittableList() {};
        HittableList(std::shared_ptr<Hittable> object) { add(object); }

        void clear() { objects.clear(); }

        void add(std::shared_ptr<Hittable> object) { 
            objects.push_back(object);
        }

        bool raymarch(const Ray& ray, HitRecord& rec) const override {
            HitRecord tempRec;
            bool hitAnything = false;
            auto closestSoFar = infinity;

            for (const auto& object : objects) {
                if (object->raymarch(ray, tempRec)) {
                    float distance = glm::length(tempRec.point - ray.origin());
                    if (distance < closestSoFar) {
                        hitAnything = true;
                        closestSoFar = distance;
                        rec = tempRec;
                    }
                }
            }

            return hitAnything;
        }

        bool shadowMarch(const Ray& ray, float lightDist) const override {
            for (const auto& object : objects) {
                if (object->shadowMarch(ray, lightDist))
                    return true;
            }
            return false;
        }

        float sdf(const glm::vec3& p) const override {
            return infinity;
        }
};

#endif