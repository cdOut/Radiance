#ifndef EXPORTER_H
#define EXPORTER_H

#include <tiny_gltf.h>
#include "entity/Entity.h"
#include "entity/util/Camera.h"
#include "entity/light/Light.h"

class Exporter {
    public:
        static bool exportToGLTF(const std::unordered_map<int, std::unique_ptr<Entity>>& entities, const std::string& path) {
            tinygltf::Model model;
            tinygltf::Scene scene;
            tinygltf::Buffer buffer;
            model.buffers.push_back(buffer);
            tinygltf::Value::Array lightsArray;
            
            for (const auto& [_, e] : entities) {
                if (Camera* camera = dynamic_cast<Camera*>(e.get())) {
                    tinygltf::Camera gltfCamera;
                    gltfCamera.type = "perspective";
                    gltfCamera.perspective.yfov = glm::radians(90.0f);
                    gltfCamera.perspective.znear = 0.1f;
                    gltfCamera.perspective.zfar = 100.0f;

                    model.cameras.push_back(gltfCamera);

                    tinygltf::Node node;
                    node.camera = model.cameras.size() - 1;

                    glm::vec3 pos = camera->getTransform().position;
                    glm::vec3 fwd = camera->getForward();
                    glm::vec3 up  = camera->getUp();
                    glm::vec3 right = camera->getRight();

                    glm::mat4 M(1.0f);
                    M[0] = glm::vec4(right, 0.0f);
                    M[1] = glm::vec4(up, 0.0f);
                    M[2] = glm::vec4(-fwd, 0.0f);
                    M[3] = glm::vec4(pos, 1.0f);

                    for (int i = 0; i < 16; i++) 
                        node.matrix.push_back(M[i/4][i%4]);

                    model.nodes.push_back(node);
                    scene.nodes.push_back(model.nodes.size() - 1);
                }

                if (Light* light = dynamic_cast<Light*>(e.get())) {
                    if (std::find(model.extensionsUsed.begin(), model.extensionsUsed.end(), "KHR_lights_punctual") == model.extensionsUsed.end()) {
                        model.extensionsUsed.push_back("KHR_lights_punctual");
                        model.extensionsRequired.push_back("KHR_lights_punctual");
                    }

                    tinygltf::Light gltfLight;
                    gltfLight.color = { light->getColor().r, light->getColor().g, light->getColor().b };
                    gltfLight.intensity = convertIntensity(light);

                    if (dynamic_cast<DirectionalLight*>(light))
                        gltfLight.type = "directional";
                    else if (dynamic_cast<PointLight*>(light))
                        gltfLight.type = "point";
                    else
                        gltfLight.type = "spot";

                    model.lights.push_back(gltfLight);
                    int lightIndex = static_cast<int>(model.lights.size() - 1);

                    tinygltf::Node node;
                    node.light = lightIndex;

                    glm::mat4 M = light->getModelMatrix();
                    for (int i = 0; i < 16; i++)
                        node.matrix.push_back(M[i / 4][i % 4]);

                    model.nodes.push_back(node);
                    scene.nodes.push_back(static_cast<int>(model.nodes.size() - 1));

                    tinygltf::Value::Object lightObj;

                    tinygltf::Value::Array colorArray;
                    colorArray.emplace_back(gltfLight.color[0]);
                    colorArray.emplace_back(gltfLight.color[1]);
                    colorArray.emplace_back(gltfLight.color[2]);
                    lightObj["color"] = tinygltf::Value(colorArray);

                    lightObj["intensity"] = tinygltf::Value(gltfLight.intensity);

                    lightObj["type"] = tinygltf::Value(gltfLight.type);

                    if (SpotLight* spotlight = dynamic_cast<SpotLight*>(light)) {
                        float size = spotlight->getSize();
                        float blend = spotlight->getBlend();

                        float outer = glm::radians(size * 0.5f);
                        float inner = outer * (1.0f - blend);

                        model.lights[lightIndex].spot = tinygltf::SpotLight();
                        model.lights[lightIndex].spot.innerConeAngle = inner;
                        model.lights[lightIndex].spot.outerConeAngle = outer;
                    }

                    lightsArray.emplace_back(tinygltf::Value(lightObj));
                }

                if (Mesh* mesh = dynamic_cast<Mesh*>(e.get())) {
                    auto& buffer = model.buffers[0];

                    auto verts = mesh->getVertices();
                    auto inds  = mesh->getIndices();

                    weldVertices(verts, inds);
                    fixWindingOrder(inds, verts);

                    const size_t vertexCount = verts.size() / 6;
                    const size_t indexCount  = inds.size();

                    size_t vertexByteOffset = buffer.data.size();

                    for (size_t i = 0; i < vertexCount; i++) {
                        float px = verts[i * 6 + 0];
                        float py = verts[i * 6 + 1];
                        float pz = verts[i * 6 + 2];

                        unsigned char* p;
                        p = reinterpret_cast<unsigned char*>(&px); buffer.data.insert(buffer.data.end(), p, p + 4);
                        p = reinterpret_cast<unsigned char*>(&py); buffer.data.insert(buffer.data.end(), p, p + 4);
                        p = reinterpret_cast<unsigned char*>(&pz); buffer.data.insert(buffer.data.end(), p, p + 4);
                    }

                    size_t normalByteOffset = buffer.data.size();

                    for (size_t i = 0; i < vertexCount; i++) {
                        float nx = verts[i * 6 + 3];
                        float ny = verts[i * 6 + 4];
                        float nz = verts[i * 6 + 5];

                        unsigned char* p;
                        p = reinterpret_cast<unsigned char*>(&nx); buffer.data.insert(buffer.data.end(), p, p + 4);
                        p = reinterpret_cast<unsigned char*>(&ny); buffer.data.insert(buffer.data.end(), p, p + 4);
                        p = reinterpret_cast<unsigned char*>(&nz); buffer.data.insert(buffer.data.end(), p, p + 4);
                    }

                    size_t indexByteOffset = buffer.data.size();

                    for (size_t i = 0; i < inds.size(); i += 3) {
                        uint32_t i0 = inds[i];
                        uint32_t i1 = inds[i + 1];
                        uint32_t i2 = inds[i + 2];

                        uint32_t p0 = i0;
                        uint32_t p1 = i2;
                        uint32_t p2 = i1;

                        unsigned char* p;
                        p = reinterpret_cast<unsigned char*>(&p0); buffer.data.insert(buffer.data.end(), p, p + 4);
                        p = reinterpret_cast<unsigned char*>(&p1); buffer.data.insert(buffer.data.end(), p, p + 4);
                        p = reinterpret_cast<unsigned char*>(&p2); buffer.data.insert(buffer.data.end(), p, p + 4);
                    }

                    tinygltf::BufferView bvVertices;
                    bvVertices.buffer     = 0;
                    bvVertices.byteOffset = vertexByteOffset;
                    bvVertices.byteLength = normalByteOffset - vertexByteOffset;
                    bvVertices.target     = TINYGLTF_TARGET_ARRAY_BUFFER;
                    bvVertices.byteStride = 3 * sizeof(float);

                    int bvVerticesIndex = model.bufferViews.size();
                    model.bufferViews.push_back(bvVertices);

                    tinygltf::BufferView bvIndices;
                    bvIndices.buffer     = 0;
                    bvIndices.byteOffset = indexByteOffset;
                    bvIndices.byteLength = indexCount * sizeof(uint32_t);
                    bvIndices.target     = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

                    int bvIndicesIndex = model.bufferViews.size();
                    model.bufferViews.push_back(bvIndices);

                    tinygltf::BufferView bvNormals;
                    bvNormals.buffer = 0;
                    bvNormals.byteOffset = normalByteOffset;
                    bvNormals.byteLength = vertexCount * 3 * sizeof(float);
                    bvNormals.target = TINYGLTF_TARGET_ARRAY_BUFFER;
                    int bvNormalsIndex = model.bufferViews.size();
                    model.bufferViews.push_back(bvNormals);

                    tinygltf::Accessor posAccessor;
                    posAccessor.bufferView    = bvVerticesIndex;
                    posAccessor.byteOffset    = 0;
                    posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
                    posAccessor.count         = vertexCount;
                    posAccessor.type          = TINYGLTF_TYPE_VEC3;
                    posAccessor.normalized    = false;

                    tinygltf::Accessor normalAccessor;
                    normalAccessor.bufferView = bvNormalsIndex;
                    normalAccessor.byteOffset = 0;
                    normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
                    normalAccessor.count = vertexCount;
                    normalAccessor.type = TINYGLTF_TYPE_VEC3;
                    normalAccessor.normalized = false;
                    int normalAccessorIndex = model.accessors.size();
                    model.accessors.push_back(normalAccessor);

                    glm::vec3 minPos(FLT_MAX), maxPos(-FLT_MAX);
                    for (size_t i = 0; i < vertexCount; i++) {
                        glm::vec3 p(verts[i * 3 + 0], verts[i * 3 + 1], verts[i * 3 + 2]);
                        minPos = glm::min(minPos, p);
                        maxPos = glm::max(maxPos, p);
                    }
                    posAccessor.minValues = {minPos.x, minPos.y, minPos.z};
                    posAccessor.maxValues = {maxPos.x, maxPos.y, maxPos.z};

                    int posAccessorIndex = model.accessors.size();
                    model.accessors.push_back(posAccessor);

                    tinygltf::Accessor idxAccessor;
                    idxAccessor.bufferView    = bvIndicesIndex;
                    idxAccessor.byteOffset    = 0;
                    idxAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
                    idxAccessor.count         = indexCount;
                    idxAccessor.type          = TINYGLTF_TYPE_SCALAR;
                    idxAccessor.normalized    = false;

                    int idxAccessorIndex = model.accessors.size();
                    model.accessors.push_back(idxAccessor);

                    tinygltf::Primitive primitive;
                    primitive.mode = TINYGLTF_MODE_TRIANGLES;
                    primitive.attributes["POSITION"] = posAccessorIndex;
                    primitive.attributes["NORMAL"] = normalAccessorIndex;
                    primitive.indices = idxAccessorIndex;

                    tinygltf::Material material;
                    material.name = "DefaultMaterial";

                    glm::vec3 baseColor = mesh->getMaterial().albedo;
                    material.pbrMetallicRoughness.baseColorFactor = { baseColor.r, baseColor.g, baseColor.b, 1.0f };

                    material.pbrMetallicRoughness.metallicFactor = mesh->getMaterial().metallic;
                    material.pbrMetallicRoughness.roughnessFactor = mesh->getMaterial().roughness;

                    material.doubleSided = true;

                    int materialIndex = model.materials.size();
                    model.materials.push_back(material);

                    primitive.material = materialIndex;

                    tinygltf::Mesh gltfMesh;
                    gltfMesh.primitives.push_back(primitive);

                    int meshIndex = model.meshes.size();
                    model.meshes.push_back(gltfMesh);

                    tinygltf::Node node;
                    node.mesh = meshIndex;

                    glm::mat4 M = mesh->getModelMatrix();
                    for (int i = 0; i < 16; i++)
                        node.matrix.push_back(M[i/4][i%4]);

                    model.nodes.push_back(node);
                    scene.nodes.push_back(model.nodes.size() - 1);
                }
            }

            if (!lightsArray.empty()) {
                tinygltf::Value::Object khrLights;
                khrLights["lights"] = tinygltf::Value(lightsArray);
                model.extensions["KHR_lights_punctual"] = tinygltf::Value(khrLights);
            }

            model.scenes.push_back(scene);
            model.defaultScene = 0;

            tinygltf::TinyGLTF gltf;
            return gltf.WriteGltfSceneToFile(&model, path, true, true, true, true);
        }
    private:
        static float convertIntensity(Light* light) {
            if (dynamic_cast<DirectionalLight*>(light))
                return light->getIntensity() * 1000.0f;
            if (dynamic_cast<PointLight*>(light))
                return light->getIntensity() * 800.0f;
            if (dynamic_cast<SpotLight*>(light))
                return light->getIntensity() * 600.0f;
            return light->getIntensity() * 10.0f;
        }

        static void fixWindingOrder(std::vector<unsigned int>& inds, const std::vector<float>& verts) {
            for (size_t i = 0; i < inds.size(); i += 3) {
                unsigned int i0 = inds[i];
                unsigned int i1 = inds[i + 1];
                unsigned int i2 = inds[i + 2];

                glm::vec3 v0(verts[i0 * 6 + 0], verts[i0 * 6 + 1], verts[i0 * 6 + 2]);
                glm::vec3 v1(verts[i1 * 6 + 0], verts[i1 * 6 + 1], verts[i1 * 6 + 2]);
                glm::vec3 v2(verts[i2 * 6 + 0], verts[i2 * 6 + 1], verts[i2 * 6 + 2]);

                glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                glm::vec3 avgNormal(
                    verts[i0 * 6 + 3] + verts[i1 * 6 + 3] + verts[i2 * 6 + 3],
                    verts[i0 * 6 + 4] + verts[i1 * 6 + 4] + verts[i2 * 6 + 4],
                    verts[i0 * 6 + 5] + verts[i1 * 6 + 5] + verts[i2 * 6 + 5]
                );
                avgNormal = glm::normalize(avgNormal);

                if (glm::dot(faceNormal, avgNormal) > 0.0f) {
                    std::swap(inds[i + 1], inds[i + 2]);
                }
            }
        }

        static void weldVertices(std::vector<float>& verts, std::vector<unsigned int>& inds) {
            const int stride = 6;

            std::vector<float> newVerts;
            std::vector<unsigned int> newInds;
            std::map<std::array<float, 6>, unsigned int> uniqueMap;

            for (size_t i = 0; i < inds.size(); i++) {
                unsigned int oldIndex = inds[i];

                std::array<float, 6> key = {
                    verts[oldIndex * stride + 0],
                    verts[oldIndex * stride + 1],
                    verts[oldIndex * stride + 2],
                    verts[oldIndex * stride + 3],
                    verts[oldIndex * stride + 4],
                    verts[oldIndex * stride + 5]
                };

                if (uniqueMap.count(key) == 0) {
                    unsigned int newIndex = newVerts.size() / stride;
                    uniqueMap[key] = newIndex;

                    for (int j = 0; j < stride; j++)
                        newVerts.push_back(key[j]);
                }

                newInds.push_back(uniqueMap[key]);
            }

            verts = std::move(newVerts);
            inds = std::move(newInds);
        }
};

#endif