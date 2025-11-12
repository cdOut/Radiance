#ifndef CUBE_H
#define CUBE_H

#include "../Mesh.h"

class Cube : public Mesh {
    public:
        virtual void generateMesh() override {
            float size = 1.0f;
            float h = size * 0.5f;

            struct Face {
                glm::vec3 n;
                glm::vec3 v0, v1, v2, v3;
            };

            std::vector<Face> faces = {
                {{0,0,1}, {-h,-h,h}, {h,-h,h}, {h,h,h}, {-h,h,h}},
                {{0,0,-1}, {h,-h,-h}, {-h,-h,-h}, {-h,h,-h}, {h,h,-h}},
                {{-1,0,0}, {-h,-h,-h}, {-h,-h,h}, {-h,h,h}, {-h,h,-h}},
                {{1,0,0}, {h,-h,h}, {h,-h,-h}, {h,h,-h}, {h,h,h}},
                {{0,1,0}, {-h,h,h}, {h,h,h}, {h,h,-h}, {-h,h,-h}},
                {{0,-1,0}, {-h,-h,-h}, {h,-h,-h}, {h,-h,h}, {-h,-h,h}}
            };

            unsigned int index = 0;

            for (const auto& f : faces) {
                for (auto& p : {f.v0, f.v1, f.v2, f.v3}) {
                    _vertices.insert(_vertices.end(), {p.x, p.y, p.z});
                    _vertices.insert(_vertices.end(), {f.n.x, f.n.y, f.n.z});
                }

                _indices.insert(_indices.end(), {
                    index, index + 1, index + 2,
                    index + 2, index + 3, index
                });

                index += 4;
            }
        }
};

#endif