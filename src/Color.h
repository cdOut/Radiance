#ifndef COLOR_H
#define COLOR_H

#include <glm/glm.hpp>
#include <iostream>

using Color = glm::vec3;

void writeColor(std::ostream& out, const Color& pixelColor) {
    auto r = pixelColor.x;
    auto g = pixelColor.y;
    auto b = pixelColor.z;

    int rbyte = int(255.999 * r);
    int gbyte = int(255.999 * g);
    int bbyte = int(255.999 * b);

    out << rbyte << " " << gbyte << " " << bbyte << "\n";
}

#endif