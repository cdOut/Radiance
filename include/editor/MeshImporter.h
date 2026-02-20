#ifndef MESHIMPORTER_H
#define MESHIMPORTER_H

#include <tiny_gltf.h>
#include "Scene.h"
#include "entity/mesh/RawMesh.h"

class MeshImporter {
    public:
        static bool importFromGLTF(Scene& scene, const std::string& path) {
            tinygltf::Model model;
            tinygltf::TinyGLTF loader;
            std::string err, warn;

            bool success = loader.LoadASCIIFromFile(&model, &err, &warn, path);
            if (!success)
                success = loader.LoadBinaryFromFile(&model, &err, &warn, path);
            if (!success)
                return false;

            if (model.meshes.empty()) return false;

            const auto& gltfMesh = model.meshes[0];
            if (gltfMesh.primitives.empty()) return false;

            const auto& primitive = gltfMesh.primitives[0];
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) return false;

            std::vector<float> vertices;
            std::vector<unsigned int> indices;

            auto posIt = primitive.attributes.find("POSITION");
            if (posIt == primitive.attributes.end()) return false;

            const tinygltf::Accessor&   posAcc = model.accessors[posIt->second];
            const tinygltf::BufferView& posBV  = model.bufferViews[posAcc.bufferView];
            const float* posData = reinterpret_cast<const float*>(
                model.buffers[posBV.buffer].data.data() + posBV.byteOffset + posAcc.byteOffset);
            size_t posStride = posBV.byteStride ? posBV.byteStride / sizeof(float) : 3;

            const float* normData = nullptr;
            size_t normStride = 3;
            auto normIt = primitive.attributes.find("NORMAL");
            if (normIt != primitive.attributes.end()) {
                const tinygltf::Accessor&   normAcc = model.accessors[normIt->second];
                const tinygltf::BufferView& normBV  = model.bufferViews[normAcc.bufferView];
                normData = reinterpret_cast<const float*>(
                    model.buffers[normBV.buffer].data.data() + normBV.byteOffset + normAcc.byteOffset);
                normStride = normBV.byteStride ? normBV.byteStride / sizeof(float) : 3;
            }

            for (size_t i = 0; i < posAcc.count; i++) {
                vertices.push_back(posData[i * posStride + 0]);
                vertices.push_back(posData[i * posStride + 1]);
                vertices.push_back(posData[i * posStride + 2]);
                if (normData) {
                    vertices.push_back(normData[i * normStride + 0]);
                    vertices.push_back(normData[i * normStride + 1]);
                    vertices.push_back(normData[i * normStride + 2]);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }
            }

            if (primitive.indices >= 0) {
                const tinygltf::Accessor&   idxAcc = model.accessors[primitive.indices];
                const tinygltf::BufferView& idxBV  = model.bufferViews[idxAcc.bufferView];
                const unsigned char* rawIdx = model.buffers[idxBV.buffer].data.data() + idxBV.byteOffset + idxAcc.byteOffset;

                for (size_t i = 0; i < idxAcc.count; i++) {
                    switch (idxAcc.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  indices.push_back(rawIdx[i]); break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: indices.push_back(reinterpret_cast<const uint16_t*>(rawIdx)[i]); break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   indices.push_back(reinterpret_cast<const uint32_t*>(rawIdx)[i]); break;
                    }
                }
            }

            std::string meshName = gltfMesh.name.empty() ? "ImportedMesh" : gltfMesh.name;
            meshName = scene.generateUniqueName(meshName);

            RawMesh* mesh = scene.createEntity<RawMesh>(std::move(vertices), std::move(indices));
            mesh->setName(meshName);

            return true;
        }
};

#endif