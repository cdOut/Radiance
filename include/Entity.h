#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>

struct Transform {
    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale {1.0f};
};

class Entity {
    public:
        virtual ~Entity() = default;
    protected:
        Transform transform;
};

#endif