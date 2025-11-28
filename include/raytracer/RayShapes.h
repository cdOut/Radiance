#ifndef RAYSHAPES_H
#define RAYSHAPES_H

#include "Hittable.h"
#include "../editor/entity/util/Transform.h"

class RaySphere : public Hittable {
    public:
        RaySphere(const Transform& transform, std::shared_ptr<RayMaterial> material) : Hittable() {
            _material = material;
            setTransform(transform);
        }

        float sdf(const glm::vec3& p) const override {
            return glm::length(p) - _radius;
        }
    private:
        float _radius = 0.5f;
};

class RayPlane : public Hittable {
    public:
        RayPlane(const Transform& transform, std::shared_ptr<RayMaterial> material) : Hittable() {
            _material = material;
            setTransform(transform);
        }

        float sdf(const glm::vec3& p) const override {
            glm::vec3 d = glm::abs(p) - glm::vec3(_halfSize, 0.0f, _halfSize);
            return glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f) + glm::length(glm::max(d, glm::vec3(0.0f)));
        }
    private:
        float _halfSize = 0.5f;
};

class RayCube : public Hittable {
    public:
        RayCube(const Transform& transform, std::shared_ptr<RayMaterial> material) : Hittable() {
            _material = material;
            setTransform(transform);
        }

        float sdf(const glm::vec3& p) const override {
            glm::vec3 d = glm::abs(p) - glm::vec3(_halfSize);
            return glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f) + glm::length(glm::max(d, glm::vec3(0.0f)));
        }
    private:
        float _halfSize = 0.5f;
};

class RayCylinder : public Hittable {
    public:
        RayCylinder(const Transform& transform, std::shared_ptr<RayMaterial> material) : Hittable() {
            _material = material;
            setTransform(transform);
        }

        float sdf(const glm::vec3& p) const override {
            glm::vec2 d = glm::vec2(glm::length(glm::vec2(p.x, p.z)) - _radius, fabs(p.y) - _halfHeight);
            return glm::min(glm::max(d.x, d.y), 0.0f) + glm::length(glm::max(d, glm::vec2(0.0f)));
        }
    private:
        float _radius = 0.5f;
        float _halfHeight = 0.5f;
};

class RayCone : public Hittable {
    public:
        RayCone(const Transform& transform, std::shared_ptr<RayMaterial> material) : Hittable() {
            _material = material;
            setTransform(transform);
        }

        float sdf(const glm::vec3& p) const override {
            float c = _radius / _height;
            glm::vec2 k = glm::normalize(glm::vec2(c, 1.0f));

            glm::vec3 pp = p;
            pp.y -= _height * 0.5f;

            glm::vec2 q = _height * glm::vec2(k.x / k.y, -1.0f);

            glm::vec2 w = glm::vec2(glm::length(glm::vec2(pp.x, pp.z)), pp.y);

            glm::vec2 a = w - q * glm::clamp(glm::dot(w, q) / glm::dot(q, q), 0.0f, 1.0f);
            glm::vec2 b = w - q * glm::vec2(glm::clamp(w.x / q.x, 0.0f, 1.0f), 1.0f);

            float ksign = glm::sign(q.y);
            float d = glm::min(glm::dot(a, a), glm::dot(b, b));
            float s = glm::max(ksign * (w.x * q.y - w.y * q.x), ksign * (w.y - q.y));

            return glm::sqrt(d) * glm::sign(s);
        }
    private:
        float _radius = 0.5f;
        float _height = 1.0f;
};

class RayTorus : public Hittable {
    public:
        RayTorus(const Transform& transform, std::shared_ptr<RayMaterial> material) : Hittable() {
            _material = material;
            setTransform(transform);
        }

        float sdf(const glm::vec3& p) const override {
            glm::vec2 q = glm::vec2(glm::length(glm::vec2(p.x, p.z)) - _majorRadius, p.y);
            return glm::length(q) - _minorRadius;
        }
    private:
        float _majorRadius = 0.5f;
        float _minorRadius = 0.25f;
};

#endif