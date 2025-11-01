#ifndef TORUS_H
#define TORUS_H

#include "Mesh.h"

class Torus : public Mesh {
    public:
        virtual void generateMesh() override {
            float majorRadius = 0.5f;
            float minorRadius = 0.25f;
            unsigned int majorSegments = 48;
            unsigned int minorSegments = 12;

            for (int i = 0; i <= majorSegments; i++) {
                float majorAngle = 2.0f * M_PI * i / majorSegments;
                for (int j = 0; j <= minorSegments; j++) {
                    float minorAngle = 2.0f * M_PI * j / minorSegments;

                    float x = (majorRadius + minorRadius * cosf(minorAngle)) * cosf(majorAngle);
                    float y = minorRadius * sinf(minorAngle);
                    float z = (majorRadius + minorRadius * cosf(minorAngle)) * sinf(majorAngle);

                    vertices.insert(vertices.end(), {x, y, z});
                }
            }

            for (int i = 0; i < majorSegments; i++) {
                for (int j = 0; j < minorSegments; j++) {
                    unsigned int k1 = i * (minorSegments + 1) + j;
                    unsigned int k2 = k1 + minorSegments + 1;

                    indices.insert(indices.end(), {k1, k2, k1 + 1});
                    indices.insert(indices.end(), {k1 + 1, k2, k2 + 1});
                }
            }
        }
};

#endif