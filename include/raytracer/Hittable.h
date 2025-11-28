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

        void setTransform(Transform& transform) {
            _transform = transform;
            calculateMatrices();
        }
    protected:
        Transform _transform;

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
};

#endif