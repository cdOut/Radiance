#ifndef IMPORTER_H
#define IMPORTER_H

#define GLM_ENABLE_EXPERIMENTAL
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Scene.h"
#include "entity/mesh/RawMesh.h"
#include "entity/mesh/primitives/Cube.h"
#include "entity/mesh/primitives/Sphere.h"
#include "entity/mesh/primitives/Plane.h"
#include "entity/mesh/primitives/Cylinder.h"
#include "entity/mesh/primitives/Cone.h"
#include "entity/mesh/primitives/Torus.h"
#include "entity/light/Light.h"
#include "entity/util/Camera.h"

class Importer {
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

            if (model.defaultScene < 0 || model.scenes.empty())
                return false;

            const tinygltf::Scene& gltfScene = model.scenes[model.defaultScene];

            for (int nodeIndex : gltfScene.nodes)
                importNode(scene, model, nodeIndex);

            return true;
        }

    private:
        static void decomposeMatrix(const std::vector<double>& mat,
                                    glm::vec3& outPos,
                                    glm::vec3& outEulerDeg,
                                    glm::vec3& outScale)
        {
            glm::mat4 M;
            for (int col = 0; col < 4; col++)
                for (int row = 0; row < 4; row++)
                    M[col][row] = static_cast<float>(mat[col * 4 + row]);

            glm::vec3 skew;
            glm::vec4 perspective;
            glm::quat rotation;
            glm::decompose(M, outScale, rotation, outPos, skew, perspective);

            glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
            outEulerDeg = euler;
        }

        static void importNode(Scene& scene, const tinygltf::Model& model, int nodeIndex) {
            const tinygltf::Node& node = model.nodes[nodeIndex];

            if (node.camera >= 0) {
                importCamera(scene, model, node);
                return;
            }

            if (node.light >= 0) {
                importLight(scene, model, node);
                return;
            }

            if (node.mesh >= 0) {
                importMesh(scene, model, node);
                return;
            }

            for (int child : node.children)
                importNode(scene, model, child);
        }

        static void importCamera(Scene& scene, const tinygltf::Model& model, const tinygltf::Node& node) {
            Camera* camera = scene.getCamera();
            if (!camera) return;

            if (!node.matrix.empty()) {
                glm::vec3 pos, eulerDeg, scale;
                decomposeMatrix(node.matrix, pos, eulerDeg, scale);

                glm::mat4 M;
                for (int col = 0; col < 4; col++)
                    for (int row = 0; row < 4; row++)
                        M[col][row] = static_cast<float>(node.matrix[col * 4 + row]);

                glm::vec3 forward = -glm::normalize(glm::vec3(M[2]));

                float pitch = glm::degrees(asinf(forward.y));
                float yaw   = glm::degrees(atan2f(forward.z, forward.x));

                camera->getTransform().position  = pos;
                camera->getTransform().rotation  = glm::vec3(pitch, yaw, 0.0f);
                camera->recalculate();
            } else if (!node.translation.empty() && !node.rotation.empty()) {
                glm::vec3 pos(node.translation[0], node.translation[1], node.translation[2]);

                glm::quat q(
                    static_cast<float>(node.rotation[3]),
                    static_cast<float>(node.rotation[0]),
                    static_cast<float>(node.rotation[1]),
                    static_cast<float>(node.rotation[2])
                );
                glm::mat4 rotMat = glm::mat4_cast(q);
                glm::vec3 forward = -glm::normalize(glm::vec3(rotMat[2]));

                float pitch = glm::degrees(asinf(forward.y));
                float yaw   = glm::degrees(atan2f(forward.z, forward.x));

                camera->getTransform().position = pos;
                camera->getTransform().rotation = glm::vec3(pitch, yaw, 0.0f);
                camera->recalculate();
            }
        }

        static void importLight(Scene& scene, const tinygltf::Model& model, const tinygltf::Node& node) {
            const tinygltf::Light& gltfLight = model.lights[node.light];

            glm::vec3 color(1.0f);
            if (gltfLight.color.size() >= 3)
                color = glm::vec3(gltfLight.color[0], gltfLight.color[1], gltfLight.color[2]);

            float intensity = static_cast<float>(gltfLight.intensity) / 5000.0f;

            glm::vec3 pos(0.0f), eulerDeg(0.0f), scale(1.0f);
            if (!node.matrix.empty())
                decomposeMatrix(node.matrix, pos, eulerDeg, scale);
            else if (!node.translation.empty())
                pos = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);

            std::string name = node.name.empty() ? gltfLight.name : node.name;

            if (gltfLight.type == "directional") {
                DirectionalLight* light = scene.createEntity<DirectionalLight>();
                applyLightTransform(light, pos, eulerDeg);
                light->getColor() = color;
                light->getIntensity() = intensity;
                if (!name.empty()) light->setName(scene.generateUniqueName(name));

            } else if (gltfLight.type == "point") {
                PointLight* light = scene.createEntity<PointLight>();
                applyLightTransform(light, pos, eulerDeg);
                light->getColor() = color;
                light->getIntensity() = intensity;
                if (!name.empty()) light->setName(scene.generateUniqueName(name));

            } else if (gltfLight.type == "spot") {
                SpotLight* light = scene.createEntity<SpotLight>();
                applyLightTransform(light, pos, eulerDeg);
                light->getColor() = color;
                light->getIntensity() = intensity;

                float outer = static_cast<float>(gltfLight.spot.outerConeAngle);
                float inner = static_cast<float>(gltfLight.spot.innerConeAngle);
                float size  = glm::degrees(outer) * 2.0f;
                float blend = (outer > 0.0f) ? (1.0f - inner / outer) : 0.0f;
                light->getSize() = size;
                light->getBlend() = blend;

                if (!name.empty()) light->setName(scene.generateUniqueName(name));
            }
        }

        static void applyLightTransform(Light* light, const glm::vec3& pos, const glm::vec3& eulerDeg) {
            light->getTransform().position = pos;
            light->getTransform().rotation = eulerDeg;
        }

        static void importMesh(Scene& scene, const tinygltf::Model& model, const tinygltf::Node& node) {
            const tinygltf::Mesh& gltfMesh = model.meshes[node.mesh];

            std::string primitiveType;
            if (gltfMesh.extras.IsObject() && gltfMesh.extras.Has("primitive")) {
                const tinygltf::Value& pv = gltfMesh.extras.Get("primitive");
                if (pv.IsString())
                    primitiveType = pv.Get<std::string>();
            }

            glm::vec3 pos(0.0f), eulerDeg(0.0f), scale(1.0f);
            if (!node.matrix.empty())
                decomposeMatrix(node.matrix, pos, eulerDeg, scale);
            else {
                if (!node.translation.empty()) pos   = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
                if (!node.scale.empty())       scale = glm::vec3(node.scale[0],       node.scale[1],       node.scale[2]);
                if (!node.rotation.empty()) {
                    glm::quat q(
                        static_cast<float>(node.rotation[3]),
                        static_cast<float>(node.rotation[0]),
                        static_cast<float>(node.rotation[1]),
                        static_cast<float>(node.rotation[2])
                    );
                    eulerDeg = glm::degrees(glm::eulerAngles(q));
                }
            }

            Material mat;
            if (!gltfMesh.primitives.empty()) {
                const tinygltf::Primitive& prim = gltfMesh.primitives[0];
                if (prim.material >= 0) {
                    const tinygltf::Material& gltfMat = model.materials[prim.material];
                    const auto& pbr = gltfMat.pbrMetallicRoughness;
                    if (pbr.baseColorFactor.size() >= 3)
                        mat.albedo = glm::vec3(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]);
                    mat.metallic  = static_cast<float>(pbr.metallicFactor);
                    mat.roughness = static_cast<float>(pbr.roughnessFactor);
                }
            }

            Mesh* mesh = nullptr;

            if      (primitiveType == "Cube")     mesh = scene.createEntity<Cube>();
            else if (primitiveType == "Sphere")   mesh = scene.createEntity<Sphere>();
            else if (primitiveType == "Plane")    mesh = scene.createEntity<Plane>();
            else if (primitiveType == "Cylinder") mesh = scene.createEntity<Cylinder>();
            else if (primitiveType == "Cone")     mesh = scene.createEntity<Cone>();
            else if (primitiveType == "Torus")    mesh = scene.createEntity<Torus>();
            else {
                auto [verts, inds] = readGeometry(model, gltfMesh);
                if (verts.empty()) return;
                mesh = scene.createEntity<RawMesh>(std::move(verts), std::move(inds));
            }

            if (!mesh) return;

            mesh->getTransform().position = pos;
            mesh->getTransform().rotation = eulerDeg;
            mesh->getTransform().scale    = scale;

            mesh->getMaterial() = mat;

            std::string name = node.name.empty() ? gltfMesh.name : node.name;
            if (!name.empty())
                mesh->setName(scene.generateUniqueName(name));
        }

        static std::pair<std::vector<float>, std::vector<unsigned int>>
        readGeometry(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh) {
            std::vector<float> vertices;
            std::vector<unsigned int> indices;

            if (gltfMesh.primitives.empty()) return {vertices, indices};
            const tinygltf::Primitive& primitive = gltfMesh.primitives[0];
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) return {vertices, indices};

            auto posIt = primitive.attributes.find("POSITION");
            if (posIt == primitive.attributes.end()) return {vertices, indices};

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

            return {vertices, indices};
        }
};

#endif