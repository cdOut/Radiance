#ifndef CAMERA_H
#define CAMERA_H

#include <glm/gtc/matrix_transform.hpp>

#include "Entity.h"

class Camera : public Entity {
    public:
        Camera(float fov, float aspect, float nearPlane, float farPlane) 
        : fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane) {
            calculateVectors();
        }

        glm::mat4 getViewMatrix() const {
            return glm::lookAt(transform.position, transform.position + forward, up);
        }

        glm::mat4 getProjectionMatrix() const {
            return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
        }

        void handleMove(const glm::vec2& moveVector, const float& deltaTime) {
            transform.position += moveVector.x * right * cameraSpeed * deltaTime;
            transform.position += moveVector.y * forward * cameraSpeed * deltaTime;
        }

        void handleLook(const glm::vec2& lookDeltaVector) {
            transform.rotation.x += lookDeltaVector.y * cameraSensitivity;
            transform.rotation.y += lookDeltaVector.x * cameraSensitivity;

            transform.rotation.x = glm::clamp(transform.rotation.x, -89.0f, 89.0f);

            calculateVectors();
        }
    private:
        float fov;
        float aspect;
        float nearPlane;
        float farPlane;

        float cameraSpeed = 1.0f;
        float cameraSensitivity = 0.1f;

        glm::vec3 forward {0.0f, 0.0f, -1.0f};
        glm::vec3 right {1.0f, 0.0f, 0.0f};
        glm::vec3 up {0.0f, 1.0f, 0.0f};

        void calculateVectors() {
            glm::vec3 direction;
            direction.x = cosf(glm::radians(transform.rotation.y)) * cosf(glm::radians(transform.rotation.x));
            direction.y = sinf(glm::radians(transform.rotation.x));
            direction.z = sinf(glm::radians(transform.rotation.y)) * cosf(glm::radians(transform.rotation.x));

            forward = glm::normalize(direction);
            right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            up = glm::normalize(glm::cross(right, forward));
        }
};

#endif