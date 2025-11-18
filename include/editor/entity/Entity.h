#ifndef ENTITY_H
#define ENTITY_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include "../Shader.h"

struct Transform {
    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale {1.0f};
};

class Entity {
    public:
        virtual ~Entity() = default;

        template<typename T, typename... Args>
        static std::unique_ptr<T> Create(Args&&... args) {
            static_assert(std::is_base_of<Entity, T>::value, "T must derive from Entity");

            auto obj = std::make_unique<T>(std::forward<Args>(args)...);
            obj->initializeCreate();
            return obj;
        }

        virtual void render() {}

        Transform& getTransform() { return _transform; }
        const std::string& getName() const { return _name; }
        const bool getIsSelected() const { return _isSelected; }
        const unsigned int getId() const { return _id; }
        const glm::vec3 getIdColor() const { return _idColor; }
        Shader* getShader() { return _shader; }
        void setName(const std::string& name) { _name = name; }
        virtual void setShader(Shader* shader) { _shader = shader; }
        void setSelectedShader(Shader* selectedShader) { _selectedShader = selectedShader; }
        virtual void setIsSelected(bool isSelected) { _isSelected = isSelected; }
        void setId(unsigned int id) { 
            _id = id;
            _idColor = {(id & 0xFF), ((id >> 8) & 0xFF), ((id >> 16) & 0xFF)};
        }

        virtual glm::mat4 getModelMatrix() const {
            glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), _transform.position);
            glm::quat rotationQuat = glm::quat(glm::yawPitchRoll(
                glm::radians(_transform.rotation.y),
                glm::radians(_transform.rotation.x),
                glm::radians(_transform.rotation.z)
            ));
            glm::mat4 rotationMat = glm::toMat4(rotationQuat);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), _transform.scale);
            return translationMat * rotationMat * scaleMat;
        }
    protected:
        unsigned int _id = 0;
        glm::vec3 _idColor{0.0f};
        Transform _transform;
        std::string _name;
        Shader* _shader = nullptr;
        Shader* _selectedShader = nullptr;
        bool _isSelected = true;

        virtual void initializeCreate() {}

        virtual glm::vec3 getForwardVector() const {
            glm::mat4 model = getModelMatrix();
            return glm::normalize(glm::vec3(model[2]) * -1.0f);
        }
};

#endif