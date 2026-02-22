#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <stb_image_write.h>

#include "util/RaytracerUtils.h"
#include "util/RayCamera.h"
#include "hittable/HittableList.h"
#include "hittable/RayShapes.h"
#include "util/RayMaterial.h"
#include "light/RayLightList.h"
#include "hittable/RayMesh.h"

#include "../editor/entity/Entity.h"
#include "../editor/entity/mesh/RawMesh.h"

class Raytracer {
    public:
        static void raytrace(const std::unordered_map<int, std::unique_ptr<Entity>>& entities, Color skyboxColor, int imageWidth, int samplesPerPixel, int maxDepth) {
            _world.clear();
            _lights.clear();

            for (const auto& [_, e] : entities) {
                Transform& transform = e->getTransform();

                if (dynamic_cast<Camera*>(e.get())) {
                    camera.transform() = transform;
                }

                if (Light* light = dynamic_cast<Light*>(e.get())) {
                    Color color = light->getColor();
                    float intensity = light->getIntensity();
                    if (dynamic_cast<DirectionalLight*>(e.get()))
                        _lights.add(std::make_shared<RayDirectionalLight>(color, intensity, transform));
                    if (dynamic_cast<PointLight*>(e.get()))
                        _lights.add(std::make_shared<RayPointLight>(color, intensity, transform));
                    if (SpotLight* spotLight = dynamic_cast<SpotLight*>(e.get()))
                        _lights.add(std::make_shared<RaySpotLight>(color, intensity, transform, spotLight->getSize(), spotLight->getBlend()));
                }

                if (Mesh* mesh = dynamic_cast<Mesh*>(e.get())) {
                    std::shared_ptr<Hittable> rayMesh;

                    Color albedo = mesh->getMaterial().albedo;
                    float metallic = mesh->getMaterial().metallic;
                    float roughness = mesh->getMaterial().roughness;
                    std::shared_ptr<RayMaterial> rayMaterial = std::make_shared<PBR>(albedo, metallic, roughness);

                    if (dynamic_cast<Sphere*>(e.get()))
                        rayMesh = std::make_shared<RaySphere>(transform, rayMaterial);
                    if (dynamic_cast<Plane*>(e.get()))
                        rayMesh = std::make_shared<RayPlane>(transform, rayMaterial);
                    if (dynamic_cast<Cube*>(e.get()))
                        rayMesh = std::make_shared<RayCube>(transform, rayMaterial);
                    if (dynamic_cast<Cylinder*>(e.get()))
                        rayMesh = std::make_shared<RayCylinder>(transform, rayMaterial);
                    if (dynamic_cast<Cone*>(e.get()))
                        rayMesh = std::make_shared<RayCone>(transform, rayMaterial);
                    if (dynamic_cast<Torus*>(e.get()))
                        rayMesh = std::make_shared<RayTorus>(transform, rayMaterial);
                    if (RawMesh* rawMesh = dynamic_cast<RawMesh*>(e.get()))
                        rayMesh = std::make_shared<RayMesh>(rawMesh->getVertices(), rawMesh->getIndices(), transform, rayMaterial);

                    if (rayMesh)
                        _world.add(rayMesh);
                }
            }

            camera.aspectRatio() = 16.0 / 9.0;
            camera.imageWidth() = imageWidth;
            camera.samplesPerPixel() = samplesPerPixel;
            camera.maxDepth() = maxDepth;
            camera.skyboxColor() = skyboxColor;

            camera.render(_world, _lights);
        }

        inline static RayCamera camera;
    private:
        inline static HittableList _world;
        inline static RayLightList _lights;
};

#endif