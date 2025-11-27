#ifndef RAYLIGHT_H
#define RAYLIGHT_H

#include "RaytracerUtils.h"
#include "../editor/entity/util/Transform.h"

class RayLight {
    public:
        virtual ~RayLight() = default;

        virtual glm::vec3 directionFrom(const glm::vec3& point) const = 0;

        virtual Color intensityAt(const glm::vec3& point) const = 0;

        virtual bool isFinite() const = 0;

        Color& color() { return _color; }
        float& intensity() { return _intensity; }
        Transform& transform() { return _transform; }
    protected:
        Color _color;
        float _intensity;
        Transform _transform;
};

class RayDirectionalLight : public RayLight {
    public:
        RayDirectionalLight(const glm::vec3& color, const float intensity, const Transform& transform) : RayLight() {
            _color = color;
            _intensity = intensity;
            _transform = transform;
        }

        glm::vec3 directionFrom(const glm::vec3& point) const override {
            return -getDirection();
        }

        Color intensityAt(const glm::vec3& point) const override {
            return _color * _intensity;
        }

        bool isFinite() const override {
            return false;
        }
    private:
        glm::vec3 getDirection() const {
            glm::quat rotationQuat = glm::quat(glm::yawPitchRoll(
                glm::radians(_transform.rotation.y),
                glm::radians(_transform.rotation.x),
                glm::radians(_transform.rotation.z)
            ));

            return glm::normalize(rotationQuat * glm::vec3(0.0f, 0.0f, -1.0f));
        }
};

class RayPointLight : public RayLight {
    public:
        RayPointLight(const glm::vec3& color, const float intensity, const Transform& transform) : RayLight() {
            _color = color;
            _intensity = intensity;
            _transform = transform;
        }

        glm::vec3 directionFrom(const glm::vec3& point) const override {
            return glm::normalize(_transform.position - point);
        }

        Color intensityAt(const glm::vec3& point) const override {
            float distance = glm::length(_transform.position - point);
            float attenuation = 1.0f / (_constant + _linear * distance + _quadratic * (distance * distance));

            return _color * _intensity * attenuation;
        }

        bool isFinite() const override {
            return true;
        }
    private:
        float _constant = 1.0f;
        float _linear = 0.09f;
        float _quadratic = 0.032f;
};

class RaySpotLight : public RayLight {
    public:
        RaySpotLight(const glm::vec3& color, const float intensity, const Transform& transform, const float size, const float blend) : RayLight(), _size(size), _blend(blend) {
            _color = color;
            _intensity = intensity;
            _transform = transform;
        }

        glm::vec3 directionFrom(const glm::vec3& point) const override {
            return glm::normalize(_transform.position - point);
        }

        Color intensityAt(const glm::vec3& point) const override {
            glm::vec3 lightDir = glm::normalize(point - _transform.position);

            float theta = glm::dot(lightDir, getDirection());
            theta = glm::clamp(theta, -1.0f, 1.0f);
            float angle = glm::acos(theta);

            float outerCutOff = glm::radians(_size * 0.5f);
            float cutOff = outerCutOff * (1.0f - _blend);

            if (angle > outerCutOff)
                return Color(0.0f);

            float epsilon = cutOff - outerCutOff;
            float spotIntensity = glm::clamp((angle - outerCutOff) / epsilon, 0.0f, 1.0f);

            float distance = glm::length(_transform.position - point);
            float attenuation = 1.0f / (_constant + _linear * distance + _quadratic * (distance * distance));

            return _color * _intensity * attenuation * spotIntensity;
        }

        bool isFinite() const override {
            return true;
        }
    private:
        float _constant = 1.0f;
        float _linear = 0.09f;
        float _quadratic = 0.032f;

        float _size = 45.0f;
        float _blend = 0.15f;

        glm::vec3 getDirection() const {
            glm::quat rotationQuat = glm::quat(glm::yawPitchRoll(
                glm::radians(_transform.rotation.y),
                glm::radians(_transform.rotation.x),
                glm::radians(_transform.rotation.z)
            ));

            return glm::normalize(rotationQuat * glm::vec3(0.0f, 0.0f, -1.0f));
        }
};

#endif