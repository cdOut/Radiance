#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform {
    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale {1.0f};
};

class Entity {
    public:
        virtual ~Entity() = default;

        const Transform& getTransform() const { return transform; }
    protected:
        Transform transform;
        
        virtual glm::mat4 getModelMatrix() const {
            glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), transform.position);
            glm::quat rotationQuat = glm::quat(glm::yawPitchRoll(
                glm::radians(transform.rotation.y),
                glm::radians(transform.rotation.x),
                glm::radians(transform.rotation.z)
            ));
            glm::mat4 rotationMat = glm::toMat4(rotationQuat);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), transform.scale);
            return translationMat * rotationMat * scaleMat;
        }
};

#endif