#ifndef SPHERE_H
#define SPHERE_H

#include "Mesh.h"

class Sphere : public Mesh {
    public:
        virtual void generateMesh() override {
            vertices.clear();
            indices.clear();

            float r = 0.5f;
            unsigned int rings = 16;
            unsigned int segments = 32;

            for (int i = 0; i <= rings; i++) {
                float ringsAngle = M_PI * i / rings;
                float y = r * cosf(ringsAngle);
                float rr = r * sinf(ringsAngle);
                for (int j = 0; j <= segments; j++) {
                    float segmentsAngle = 2.0f * M_PI * j / segments;
                    float x = rr * cosf(segmentsAngle);
                    float z = rr * sinf(segmentsAngle);
                    vertices.insert(vertices.end(), {x, y, z});
                }
            }

            for (int i = 0; i < rings; i++) {
                for (int j = 0; j < segments; j++) {
                    unsigned int k1 = i * (segments + 1) + j;
                    unsigned int k2 = k1 + segments + 1;

                    indices.insert(indices.end(), {k1, k2, k1 + 1});
                    indices.insert(indices.end(), {k1 + 1, k2, k2 + 1});
                }
            }
        }
};

#endif