#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <stb_image_write.h>

#include "RaytracerUtils.h"
#include "RayCamera.h"
#include "HittableList.h"
#include "RayShapes.h"
#include "RayMaterial.h"
#include "RayLightList.h"

#include "../editor/entity/Entity.h"

class Raytracer {
    public:
        static std::vector<unsigned char> raytrace(const std::unordered_map<int, std::unique_ptr<Entity>>& entities, Color skyboxColor, int imageWidth, int samplesPerPixel, int maxDepth) {
            _world.clear();
            _lights.clear();

            auto materialCenter = std::make_shared<Lambertian>(Color(0.1, 0.2, 0.5));

            for (const auto& [_, e] : entities) {
                Transform& transform = e->getTransform();

                if (dynamic_cast<Camera*>(e.get())) {
                    _camera.transform() = transform;
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

                std::shared_ptr<Hittable> rayMesh;

                if (dynamic_cast<Sphere*>(e.get()))
                    rayMesh = std::make_shared<RaySphere>(transform, materialCenter);
                if (dynamic_cast<Plane*>(e.get()))
                    rayMesh = std::make_shared<RayPlane>(transform, materialCenter);
                if (dynamic_cast<Cube*>(e.get()))
                    rayMesh = std::make_shared<RayCube>(transform, materialCenter);
                if (dynamic_cast<Cylinder*>(e.get()))
                    rayMesh = std::make_shared<RayCylinder>(transform, materialCenter);
                if (dynamic_cast<Cone*>(e.get()))
                    rayMesh = std::make_shared<RayCone>(transform, materialCenter);
                if (dynamic_cast<Torus*>(e.get()))
                    rayMesh = std::make_shared<RayTorus>(transform, materialCenter);
                
                if (rayMesh)
                    _world.add(rayMesh);
            }

            _camera.aspectRatio() = 16.0 / 9.0;
            _camera.imageWidth() = imageWidth;
            _camera.samplesPerPixel() = samplesPerPixel;
            _camera.maxDepth() = maxDepth;
            _camera.skyboxColor() = skyboxColor;

            return _camera.render(_world, _lights);
        }

        static unsigned int uploadRender(const unsigned char* data) {
            static unsigned int renderId = 0;

            if (!renderId)
                glGenTextures(1, &renderId);

            glBindTexture(GL_TEXTURE_2D, renderId);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _camera.imageWidth(), _camera.imageHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);

            return renderId;
        }
    private:
        inline static HittableList _world;
        inline static RayLightList _lights;
        inline static RayCamera _camera;
};

#endif