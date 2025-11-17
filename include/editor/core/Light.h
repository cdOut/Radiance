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

        glm::vec3& getColor() { return _color; }
        float& getIntensity() { return _intensity; }

        virtual void uploadToShader(Shader* shader, int index) = 0;

        virtual void render() override {
            glm::mat4 model = getModelMatrix();

            _billboard.render(model);
        }
    protected:
        Billboard _billboard;
        LightType _type;

        unsigned int _texture, _selectedTexture;

        glm::vec3 _color{1.0f, 1.0f, 1.0f};
        float _intensity = 1.0f;
};

class DirectionalLight : public Light {
    public:
        DirectionalLight() : Light(LightType::Directional) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "directionalLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".direction", getForwardVector());
            shader->setVec3(arrayString + ".ambient", _color * _intensity * 0.1f);
            shader->setVec3(arrayString + ".diffuse", _color * _intensity);
            shader->setVec3(arrayString + ".specular", _color * _intensity);
        };
};

class PointLight : public Light {
    public:
        PointLight() : Light(LightType::Point) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "pointLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".position", glm::vec3(getModelMatrix()[3]));
            shader->setVec3(arrayString + ".ambient", _color * _intensity * 0.1f);
            shader->setVec3(arrayString + ".diffuse", _color * _intensity);
            shader->setVec3(arrayString + ".specular", _color * _intensity);

            shader->setFloat(arrayString + ".constant", _constant);
            shader->setFloat(arrayString + ".linear", _linear);
            shader->setFloat(arrayString + ".quadratic", _quadratic);
        };
    protected:
        float _constant = 1.0f;
        float _linear = 0.09f;
        float _quadratic = 0.032f;
};

class SpotLight : public Light {
    public:
        SpotLight() : Light(LightType::Spot) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "spotLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".position", glm::vec3(getModelMatrix()[3]));
            shader->setVec3(arrayString + ".direction", getForwardVector());
            shader->setVec3(arrayString + ".ambient", _color * _intensity * 0.1f);
            shader->setVec3(arrayString + ".diffuse", _color * _intensity);
            shader->setVec3(arrayString + ".specular", _color * _intensity);

            shader->setFloat(arrayString + ".constant", _constant);
            shader->setFloat(arrayString + ".linear", _linear);
            shader->setFloat(arrayString + ".quadratic", _quadratic);

            float outerCutOff = glm::radians(_size * 0.5f);
            float cutOff = outerCutOff * (1.0f - _blend);

            shader->setFloat(arrayString + ".cutOff", cosf(cutOff));
            shader->setFloat(arrayString + ".outerCutOff", cosf(outerCutOff));
        };

        float& getSize() { return _size; }
        float& getBlend() { return _blend; }
    protected:
        float _constant = 1.0f;
        float _linear = 0.09f;
        float _quadratic = 0.032f;

        float _size = 45.0f;
        float _blend = 0.15f;
};

#endif