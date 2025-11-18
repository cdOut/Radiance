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
        DirectionalLight() : Light(LightType::Directional) {
            glGenVertexArrays(1, &_VAO);
            glGenBuffers(1, &_VBO);
            
            _vertices[0] = _vertices[2] = {};
            _vertices[1] = _vertices[3] = {0.0f, 1.0f, 0.0f};

            glBindVertexArray(_VAO);
            glBindBuffer(GL_ARRAY_BUFFER, _VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(_vertices), _vertices, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*) (sizeof(glm::vec3)));

            glBindVertexArray(0);
        }

        ~DirectionalLight() {
            if (_VAO) glDeleteVertexArrays(1, &_VAO);
            if (_VBO) glDeleteBuffers(1, &_VBO);
        }

        virtual void render() override {
            Light::render();

            if (_isSelected && _selectedShader) {
                glm::vec3 start = glm::vec3(getModelMatrix()[3]);
                glm::vec3 dir = glm::normalize(getForwardVector());
                glm::vec3 end = start + dir * 4.0f;

                _vertices[0] = start;
                _vertices[2] = end;

                glBindVertexArray(_VAO);
                glBindBuffer(GL_ARRAY_BUFFER, _VBO);
                glBufferData(GL_ARRAY_BUFFER, sizeof(_vertices), _vertices, GL_DYNAMIC_DRAW);

                _selectedShader->use();
                _selectedShader->setMat4("model", glm::mat4(1.0f));
                _selectedShader->setMat3("normalMatrix", glm::mat4(1.0f));
                _selectedShader->setVec3("color", glm::vec3(1.0f));

                glDrawArrays(GL_LINES, 0, 2);

                glBindVertexArray(0);
            }
        }

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "directionalLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".direction", getForwardVector());
            shader->setVec3(arrayString + ".color", _color * _intensity);
        };
    protected:
        unsigned int _VAO, _VBO;
        glm::vec3 _vertices[4];
};

class PointLight : public Light {
    public:
        PointLight() : Light(LightType::Point) {}

        virtual void uploadToShader(Shader* shader, int index) override {
            std::string arrayString = "pointLights[" + std::to_string(index) + "]";

            shader->setVec3(arrayString + ".position", glm::vec3(getModelMatrix()[3]));
            shader->setVec3(arrayString + ".color", _color * _intensity);

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
            shader->setVec3(arrayString + ".color", _color * _intensity);

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