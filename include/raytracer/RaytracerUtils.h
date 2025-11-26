#ifndef RAYTRACERUTILS_H
#define RAYTRACERUTILS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>
#include <cstdlib>
#include <random>
#include <memory>
#include <limits>

#include "Ray.h"
#include "Interval.h"

const float pi = 3.1415926535897932385;

inline float randomFloat() {
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline float randomFloat(float min, float max) {
    return min + (max - min) * randomFloat();
}

inline glm::vec3 randomVec3() {
    return glm::vec3(randomFloat(), randomFloat(), randomFloat());
}

inline glm::vec3 randomVec3(float min, float max) {
    return glm::vec3(randomFloat(min, max), randomFloat(min, max), randomFloat(min, max));
}

inline glm::vec3 randomUnitVector() {
    while (true) {
        glm::vec3 p = randomVec3(-1.0f, 1.0f);
        float lensq = glm::dot(p, p);
        if (1e-160 < lensq && lensq <= 1)
            return p / std::sqrt(lensq);
    }
}

inline glm::vec3 randomOnHemisphere(const glm::vec3& normal) {
    glm::vec3 onUnitSphere = randomUnitVector();
    if (glm::dot(onUnitSphere, normal) > 0.0f)
        return onUnitSphere;
    else
        return -onUnitSphere;
}

inline float linearToGamma(float linearComponent) {
    if (linearComponent > 0)
        return std::sqrt(linearComponent);

    return 0.0f;
}

using Color = glm::vec3;

#endif