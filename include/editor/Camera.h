#ifndef CAMERA_H
#define CAMERA_H

#include <glm/gtc/matrix_transform.hpp>

#include "core/Entity.h"

class Camera : public Entity {
    public:
        Camera() : _fov(90.0f), _aspect(16.0f/9.0f), _nearPlane(0.1f), _farPlane(100.0f) {
            setDefaultTransform();
            calculateVectors();
        }

        glm::mat4 getViewMatrix() const {
            return glm::lookAt(transform.position, transform.position + _forward, _up);
        }

        glm::mat4 getProjectionMatrix() const {
            return glm::perspective(glm::radians(_fov), _aspect, _nearPlane, _farPlane);
        }

        void handleMove(const glm::vec2& moveVector, const float& deltaTime) {
            transform.position += moveVector.x * _right * _cameraSpeed * deltaTime;
            transform.position += moveVector.y * _forward * _cameraSpeed * deltaTime;
        }

        void handleLook(const glm::vec2& lookDeltaVector) {
            transform.rotation.x += lookDeltaVector.y * _cameraSensitivity;
            transform.rotation.y += lookDeltaVector.x * _cameraSensitivity;

            transform.rotation.x = glm::clamp(transform.rotation.x, -89.0f, 89.0f);

            calculateVectors();
        }

        void setAspect(float aspect) {
            _aspect = aspect;
        }
    private:
        float _fov;
        float _aspect;
        float _nearPlane;
        float _farPlane;

        float _cameraSpeed = 2.0f;
        float _cameraSensitivity = 0.2f;

        glm::vec3 _forward {0.0f, 0.0f, -1.0f};
        glm::vec3 _right {1.0f, 0.0f, 0.0f};
        glm::vec3 _up {0.0f, 1.0f, 0.0f};

        void setDefaultTransform() {
            transform.position = glm::vec3(-2.0f, 2.0f, 2.0f);
            transform.rotation = glm::vec3(-45.0f, -45.0f, 0.0f);
        }

        void calculateVectors() {
            glm::vec3 direction;
            direction.x = cosf(glm::radians(transform.rotation.y)) * cosf(glm::radians(transform.rotation.x));
            direction.y = sinf(glm::radians(transform.rotation.x));
            direction.z = sinf(glm::radians(transform.rotation.y)) * cosf(glm::radians(transform.rotation.x));

            _forward = glm::normalize(direction);
            _right = glm::normalize(glm::cross(_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            _up = glm::normalize(glm::cross(_right, _forward));
        }
};

#endif