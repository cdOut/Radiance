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

using Color = glm::vec3;

#endif