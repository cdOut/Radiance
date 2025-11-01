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

            for (const auto& vert : v) {
                vertices.insert(vertices.end(), {vert.x, vert.y, vert.z});
            }

            indices = {
                0, 1, 2,
                2, 3, 1,
            };
        }
};

#endif