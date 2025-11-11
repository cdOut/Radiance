#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>

class Ray {
    public:
        Ray() {}
        Ray(const glm::vec3& origin, const glm::vec3& direction) : _origin(origin), _direction(direction) {}

        const glm::vec3& origin() const { return _origin; }
        const glm::vec3& direction() const { return _direction; }

        glm::vec3 at(float t) const {
            return _origin + t * _direction;
        }
    private:
        glm::vec3 _origin, _direction;
};

#endif