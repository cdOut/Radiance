#ifndef CYLINDER_H
#define CYLINDER_H

#include "Mesh.h"

class Cylinder : public Mesh {
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
                vertices.insert(vertices.end(), {x, height * 0.5f, z});
            }
            vertices.insert(vertices.end(), {0.0f, -height * 0.5f, 0.0f});
            vertices.insert(vertices.end(), {0.0f, height * 0.5f, 0.0f});

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                unsigned int b0 = i * 2;
                unsigned int t0 = b0 + 1;
                unsigned int b1 = next * 2;
                unsigned int t1 = b1 + 1;
                indices.insert(indices.end(), {b0, t0, b1});
                indices.insert(indices.end(), {t0, t1, b1});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                indices.insert(indices.end(), {segments * 2, i * 2, next * 2});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                indices.insert(indices.end(), {segments * 2 + 1, i * 2 + 1, next * 2 + 1});
            }
        }
};

#endif