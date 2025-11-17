#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <string>
#include <functional>
#include "Entity.h"
#include "Scene.h"
#include "../Billboard.h"

enum class LightType {
    Directional,
    Point,
    Spot,
    END
};

class Light : public Entity {
    public:
        Light(LightType type = LightType::Directional) : _type(type) {}

        const LightType& getType() const { return _type; }

        void setTextures(unsigned int texture, unsigned int selectedTexture) {
            _texture = texture;
            _selectedTexture = selectedTexture;
            setTexture(_texture);
        }

        void setTexture(unsigned int texture) {
            _billboard.setTexture(texture);
        }

        virtual void setShader(Shader* shader) override {
            Entity::setShader(shader);
            _billboard.setShader(shader);
        }

        virtual void setIsSelected(bool isSelected) override {
            Entity::setIsSelected(isSelected);
            _billboard.setTexture(isSelected ? _selectedTexture : _texture);
        }

        virtual void uploadToShader(Shader* shader, int index) = 0;

        virtual void render() override {
            glm::mat4 model = getModelMatrix();

            _billboard.render(model);
        }
    protected:
        Billboard _billboard;
        LightType _type;

        unsigned int _texture, _selectedTexture;

        glm::vec3 ambient{0.1f, 0.1f, 0.1f};
        glm::vec3 diffuse{1.0f, 1.0f, 1.0f};
        glm::vec3 specular{1.0f, 1.0f, 1.0f};
};

class DirectionalLight : public Light {
    public:
        DirectionalLight() : Light(LightType::Directional) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "directionalLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".direction", getForwardVector());
            shader->setVec3(arrayString + ".ambient", ambient);
            shader->setVec3(arrayString + ".diffuse", diffuse);
            shader->setVec3(arrayString + ".specular", specular);
        };
};

class PointLight : public Light {
    public:
        PointLight() : Light(LightType::Point) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "pointLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".position", glm::vec3(getModelMatrix()[3]));
            shader->setVec3(arrayString + ".ambient", ambient);
            shader->setVec3(arrayString + ".diffuse", diffuse);
            shader->setVec3(arrayString + ".specular", specular);

            shader->setFloat(arrayString + ".constant", constant);
            shader->setFloat(arrayString + ".linear", linear);
            shader->setFloat(arrayString + ".quadratic", quadratic);
        };
    protected:
        float constant = 1.0f;
        float linear = 0.09f;
        float quadratic = 0.032f;
};

class SpotLight : public Light {
    public:
        SpotLight() : Light(LightType::Spot) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "spotLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".position", glm::vec3(getModelMatrix()[3]));
            shader->setVec3(arrayString + ".direction", getForwardVector());
            shader->setVec3(arrayString + ".ambient", ambient);
            shader->setVec3(arrayString + ".diffuse", diffuse);
            shader->setVec3(arrayString + ".specular", specular);

            shader->setFloat(arrayString + ".constant", constant);
            shader->setFloat(arrayString + ".linear", linear);
            shader->setFloat(arrayString + ".quadratic", quadratic);

            shader->setFloat(arrayString + ".cutOff", cutOff);
            shader->setFloat(arrayString + ".outerCutOff", outerCutOff);
        };
    protected:
        float constant = 1.0f;
        float linear = 0.09f;
        float quadratic = 0.032f;
        float cutOff = glm::cos(glm::radians(12.5f));
        float outerCutOff = glm::cos(glm::radians(17.5f));
};

#endif