#ifndef CONE_H
#define CONE_H

#include "../Mesh.h"

class Cone : public Mesh {
    public:
        virtual void generateMesh() override {
            float r = 0.5f;
            float height = 1.0f;
            unsigned int segments = 32;

            float length = sqrtf(r*r + height*height);
            float ny  = height / length;
            float nxz = r / length;

            for (int i = 0; i < segments; i++) {
                float angle = 2.0f * M_PI * i / segments;
                float x = r * cosf(angle);
                float z = r * sinf(angle);
                _vertices.insert(_vertices.end(), {x, -height * 0.5f, z});

                float nx = cosf(angle) * nxz;
                float nz = sinf(angle) * nxz;
                _vertices.insert(_vertices.end(), {nx, ny, nz});
            }

            unsigned int rimOffset = (unsigned int)_vertices.size() / 6;

            // duplicate rim vertices for additional normals to fix shading
            for (int i = 0; i < segments; i++) {
                float angle = 2.0f * M_PI * i / segments;
                float x = r * cosf(angle);
                float z = r * sinf(angle);
                _vertices.insert(_vertices.end(), {x, -height * 0.5f, z});
                _vertices.insert(_vertices.end(), {0.0f, -1.0f, 0.0f});
            }

            unsigned int bottomCenter = _vertices.size() / 6;
            _vertices.insert(_vertices.end(), {0.0f, -height * 0.5f, 0.0f});
            _vertices.insert(_vertices.end(), {0.0f, -1.0f, 0.0f});

            unsigned int topPoint = _vertices.size() / 6;
            _vertices.insert(_vertices.end(), {0.0f, height * 0.5f, 0.0f});
            _vertices.insert(_vertices.end(), {0.0f, 1.0f, 0.0f});

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                _indices.insert(_indices.end(), {bottomCenter, rimOffset + next, rimOffset + i});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                _indices.insert(_indices.end(), {i, next, topPoint});
            }
        }
};

#endif