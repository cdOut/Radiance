#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <string>
#include "Entity.h"
#include "../Billboard.h"

enum class LightType {
    Directional,
    Point,
    Spot,
    END
};

std::string getLightTypeName(LightType type) {
    switch (type) {
        case LightType::Directional: return "Directional";
        case LightType::Point: return "Point";
        case LightType::Spot: return "Spotlight";
        default: return "";
    }
}

class Light : public Entity {
    public:
        Light(LightType type = LightType::Directional) : _type(type) {}

        const LightType& getType() const { return _type; }

        void setTexture(unsigned int texture) {
            _billboard.setTexture(texture);
        }

        virtual void setShader(Shader* shader) override {
            Entity::setShader(shader);
            _billboard.setShader(shader);
        }

        virtual void render() override {
            glm::mat4 model = getModelMatrix();

            _billboard.render(model);
        }
    private:
        Billboard _billboard;
        LightType _type;

        glm::vec3 ambient{0.0f};
        glm::vec3 diffuse{0.0f};
        glm::vec3 specular{0.0f};
};

#endif