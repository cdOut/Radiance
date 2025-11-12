#ifndef PLANE_H
#define PLANE_H

#include "Mesh.h"

class Plane : public Mesh {
    public:
        virtual void generateMesh() override {
            float size = 1.0f;
            float h = size * 0.5f;

            std::vector<glm::vec3> v = {
                {-h, 0.0f, h},
                {h, 0.0f, h},
                {-h, 0.0f, -h},
                {h, 0.0f, -h},
            };

            glm::vec3 normal = {0.0f, 1.0f, 0.0f};

            for (const auto& vert : v) {
                vertices.insert(vertices.end(), {vert.x, vert.y, vert.z, normal.x, normal.y, normal.z});
            }

            indices = {
                0, 1, 2,
                2, 3, 1,
            };
        }
};

#endif