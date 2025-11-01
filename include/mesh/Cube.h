#ifndef CUBE_H
#define CUBE_H

#include "Mesh.h"

class Cube : public Mesh {
    public:
        virtual void generateMesh() override {
            float size = 1.0f;
            float h = size * 0.5f;

            std::vector<glm::vec3> v = {
                {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, { -h,  h, -h},
                {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, { -h,  h,  h}
            };

            for (const auto& vert : v) {
                vertices.insert(vertices.end(), {vert.x, vert.y, vert.z});
            }

            indices = {
                0, 1, 2,  2, 3, 0,
                4, 5, 6,  6, 7, 4,
                4, 0, 3,  3, 7, 4,
                1, 5, 6,  6, 2, 1,
                3, 2, 6,  6, 7, 3,
                4, 5, 1,  1, 0, 4
            };
        }
};

#endif