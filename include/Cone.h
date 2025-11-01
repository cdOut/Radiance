#ifndef CONE_H
#define CONE_H

#include "Mesh.h"

class Cone : public Mesh {
    public:
        Cone () {
            float r = 0.5f;
            unsigned int segments = 16;

            for (size_t i = 0; i < segments; i++) {
                float angle = 2.0f * M_PI * i / segments;
                float x = r * cosf(angle);
                float z = r * sinf(angle);
                vertices.insert(vertices.end(), {x, -r, z});
            }
            vertices.insert(vertices.end(), {0.0f, r, 0.0f});
            vertices.insert(vertices.end(), {0.0f, -r, 0.0f});

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int ii = (i + 1) % segments;
                indices.insert(indices.end(), {i, ii, segments});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int ii = (i + 1) % segments;
                indices.insert(indices.end(), {segments + 1, ii, i});
            }

            setupMesh();
        }
};

#endif