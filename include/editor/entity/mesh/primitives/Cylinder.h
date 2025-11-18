#ifndef CYLINDER_H
#define CYLINDER_H

#include "../Mesh.h"

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

                glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

                _vertices.insert(_vertices.end(), {x, -height * 0.5f, z});
                _vertices.insert(_vertices.end(), {normal.x, normal.y, normal.z});

                _vertices.insert(_vertices.end(), {x, height * 0.5f, z});
                _vertices.insert(_vertices.end(), {normal.x, normal.y, normal.z});
            }

            unsigned int bottomRim = (segments * 2);
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

            unsigned int topRim = _vertices.size() / 6;
            for (int i = 0; i < segments; i++) {
                float angle = 2.0f * M_PI * i / segments;
                float x = r * cosf(angle);
                float z = r * sinf(angle);

                _vertices.insert(_vertices.end(), {x, height * 0.5f, z});
                _vertices.insert(_vertices.end(), {0.0f, 1.0f, 0.0f});
            }

            unsigned int topCenter = _vertices.size() / 6;
            _vertices.insert(_vertices.end(), {0.0f, height * 0.5f, 0.0f});
            _vertices.insert(_vertices.end(), {0.0f, 1.0f, 0.0f});

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                unsigned int b0 = i * 2;
                unsigned int t0 = b0 + 1;
                unsigned int b1 = next * 2;
                unsigned int t1 = b1 + 1;
                _indices.insert(_indices.end(), {b0, t0, b1});
                _indices.insert(_indices.end(), {t0, t1, b1});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                _indices.insert(_indices.end(), {bottomCenter, bottomRim + next, bottomRim + i});
            }

            for (unsigned int i = 0; i < segments; i++) {
                unsigned int next = (i + 1) % segments;
                _indices.insert(_indices.end(), {topCenter, topRim + i, topRim + next});
            }
        }
};

#endif