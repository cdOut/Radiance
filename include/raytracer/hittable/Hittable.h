#ifndef HITTABLE_H
#define HITTABLE_H

#include "../util/RaytracerUtils.h"

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

        virtual float sdf(const glm::vec3& localPoint) const = 0;

        virtual bool raymarch(const Ray& ray, HitRecord& rec) const {
            const float maxDistance = 100.0f;
            const float epsilon = 1e-3f;
            const int maxSteps = 50;

            glm::vec3 o = glm::vec3(_modelMatrixI * glm::vec4(ray.origin(), 1.0f));
            glm::vec3 d = glm::normalize(glm::vec3(_modelMatrixI * glm::vec4(ray.direction(), 0.0f)));

            if (!intersectsAABB(o, d)) return false;

            float t = 0.0f;
            for (int i = 0; i < maxSteps; i++) {
                glm::vec3 p = o + d * t;
                float distance = sdf(p);

                if (distance < epsilon) {
                    rec.t = t;
                    rec.point = glm::vec3(_modelMatrix * glm::vec4(p, 1.0f));
                    glm::vec3 n = getNormal(p);
                    rec.normal = glm::normalize(glm::vec3(_modelMatrixIT * glm::vec4(n, 0.0f)));
                    rec.setFaceNormal(ray, rec.normal);
                    rec.material = _material;
                    return true;
                }

                if (t > maxDistance)
                    break;

                t += distance;
            }

            return false;
        }

        void setTransform(const Transform& transform) {
            _transform = transform;
            calculateMatrices();
        }
    protected:
        Transform _transform;
        std::shared_ptr<RayMaterial> _material;

        glm::mat4 _modelMatrix{1.0f};
        glm::mat4 _modelMatrixI{1.0f};
        glm::mat4 _modelMatrixIT{1.0f};

        void calculateMatrices() {
            glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), _transform.position);
            glm::quat rotationQuat = glm::quat(glm::yawPitchRoll(
                glm::radians(_transform.rotation.y),
                glm::radians(_transform.rotation.x),
                glm::radians(_transform.rotation.z)
            ));
            glm::mat4 rotationMat = glm::toMat4(rotationQuat);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), _transform.scale);

            _modelMatrix = translationMat * rotationMat * scaleMat;
            _modelMatrixI = glm::inverse(_modelMatrix);
            _modelMatrixIT = glm::transpose(_modelMatrixI);
        }

        glm::vec3 getNormal(const glm::vec3& p) const {
            const float h = 0.0005f;
            return glm::normalize(glm::vec3(
                sdf(p + glm::vec3(h, 0, 0)) - sdf(p - glm::vec3(h, 0, 0)),
                sdf(p + glm::vec3(0, h, 0)) - sdf(p - glm::vec3(0, h, 0)),
                sdf(p + glm::vec3(0, 0, h)) - sdf(p - glm::vec3(0, 0, h))
            ));
        }

        bool intersectsAABB(const glm::vec3& o, const glm::vec3& d) const {
            glm::vec3 min(-1.0f);
            glm::vec3 max( 1.0f);

            glm::vec3 invDir = 1.0f / d;

            glm::vec3 t0 = (min - o) * invDir;
            glm::vec3 t1 = (max - o) * invDir;

            float tmin = std::max(std::max(std::min(t0.x, t1.x), std::min(t0.y, t1.y)), std::min(t0.z, t1.z));
            float tmax = std::min(std::min(std::max(t0.x, t1.x), std::max(t0.y, t1.y)), std::max(t0.z, t1.z));

            return tmax >= tmin && tmax > 0.0f;
        }
};

#endif