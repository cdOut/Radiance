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

inline float randomFloat() {
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    static thread_local std::mt19937 generator(std::random_device{}());
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

inline glm::vec3 randomCosineHemisphere(const glm::vec3& normal) {
    float r1 = randomFloat();
    float r2 = randomFloat();

    float phi = 2.0f * glm::pi<float>() * r1;
    float sqrtR2 = std::sqrt(r2);

    float x = std::cos(phi) * sqrtR2;
    float y = std::sin(phi) * sqrtR2;
    float z = std::sqrt(1.0f - r2);

    glm::vec3 up = std::fabs(normal.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
    glm::vec3 bitangent = glm::cross(normal, tangent);

    return glm::normalize(tangent * x + bitangent * y + normal * z);
}

inline float linearToGamma(float linearComponent) {
    if (linearComponent > 0)
        return std::sqrt(linearComponent);

    return 0.0f;
}

inline bool isVectorNearZero(glm::vec3& vector) {
    auto s = 1e-8;
    return (std::fabs(vector.x) < s) && (std::fabs(vector.y) < s) && (std::fabs(vector.z) < s);
}

inline glm::vec3 reflect(const glm::vec3& vector, const glm::vec3& normal) {
    return vector - 2.0f * glm::dot(vector, normal) * normal;
}

using Color = glm::vec3;

#endif
