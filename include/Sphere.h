#ifndef SPHERE_H
#define SPHERE_H

#include "Mesh.h"

class Sphere : public Mesh {
    public:
        Sphere() {
            floatsPerVert = 3;

            float radius = 0.5f;
            unsigned int segments = 32;

            for (size_t i = 0; i <= segments; i++) {
                float verticalAngle = M_PI / 2.0 - i * segments;
                float xy = radius * cosf(verticalAngle);
                float z = radius * sinf(verticalAngle);
                
                for (size_t j = 0; j <= segments; j++) {
                    float horizontalAngle = j * verticalAngle;
                    float vx = xy * cosf(horizontalAngle);
                    float vy = xy * sinf(horizontalAngle);
                    float vz = z;
                    vertices.insert(vertices.end(), {vx, vy, vz});
                }
            }

            unsigned int k1, k2;
            for (size_t i = 0; i < segments; i++) {
                k1 = i * (segments + 1);
                k2 = k1 + segments + 1;

                for (size_t j = 0; j < segments; j++, k1++, k2++) {
                    if (i != 0)
                        indices.insert(indices.end(), {k1, k2, k1 + 1});

                    if (i != (segments - 1))
                        indices.insert(indices.end(), {k1 + 1, k2, k2 + 1});
                }
            }

            setupMesh();
        }
};

#endif