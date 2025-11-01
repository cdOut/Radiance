#ifndef CONE_H
#define CONE_H

#include "Mesh.h"

class Cone : public Mesh {
    public:
        virtual void generateMesh() override {
            float r = 0.5f;
            float height = 1.0f;
            unsigned int segments = 32;

            for (int i = 0; i < segments; i++) {
                float angle = 2.0f * M_PI * i / segments;
                float x = r * cosf(angle);
                float z = r * sinf(angle);
                vertices.insert(vertices.end(), {x, -height * 0.5f, z});
            }
            vertices.insert(vertices.end(), {0.0f, -height * 0.5f, 0.0f});
            vertices.insert(vertices.end(), {0.0f, height * 0.5f, 0.0f});

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                indices.insert(indices.end(), {segments, next, i});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                indices.insert(indices.end(), {i, next, segments + 1});
            }
        }
};

#endif