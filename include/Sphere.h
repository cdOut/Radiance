#ifndef SPHERE_H
#define SPHERE_H

#include "Mesh.h"

class Sphere : public Mesh {
    public:
        Sphere() {
            floatsPerVert = 3;

            float radius = 0.5f;
            unsigned int segments = 32;
            unsigned int rings = 16;

            for (size_t i = 0; i <= rings; i++) {
                float vAngle = M_PI * i / rings;
                float y = radius * cosf(vAngle);
                float r = radius * sinf(vAngle);
                
                for (size_t j = 0; j <= segments; j++) {
                    float hAngle = 2.0f * M_PI * j / segments;
                    float x = r * cosf(hAngle);
                    float z = r * sinf(hAngle);
                    vertices.insert(vertices.end(), {x, y, z});
                }
            }

            for (unsigned int i = 0; i < segments; i++) {
                for (unsigned int j = 0; j < segments; j++) {
                    unsigned int k1 = i * (segments + 1) + j;
                    unsigned int k2 = k1 + segments + 1;

                    indices.insert(indices.end(), {k1, k2, k1 + 1});
                    indices.insert(indices.end(), {k1 + 1, k2, k2 + 1});
                }
            }

            setupMesh();
        }
};

#endif