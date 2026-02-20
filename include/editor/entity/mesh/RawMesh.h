#ifndef RAW_MESH_H
#define RAW_MESH_H

#include "Mesh.h"

class RawMesh : public Mesh {
public:
    RawMesh(std::vector<float> verts, std::vector<unsigned int> inds) {
        _vertices = std::move(verts);
        _indices  = std::move(inds);
    }
protected:
    void generateMesh() override {}
};

#endif