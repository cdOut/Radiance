#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <stb_image_write.h>

#include "RaytracerUtils.h"
#include "RayCamera.h"
#include "HittableList.h"
#include "RaySphere.h"
#include "RayMaterial.h"
#include "RayLightList.h"

#include "../editor/entity/Entity.h"

class Raytracer {
    public:
        static std::vector<unsigned char> raytrace(const std::unordered_map<int, std::unique_ptr<Entity>>& entities, Color skyboxColor) {
            _world.clear();
            _lights.clear();

            auto materialGround = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0));
            auto materialCenter = std::make_shared<Lambertian>(Color(0.1, 0.2, 0.5));
            auto materialLeft   = std::make_shared<Metal>(Color(0.8, 0.8, 0.8));
            auto materialRight  = std::make_shared<Metal>(Color(0.8, 0.6, 0.2));

            _world.add(std::make_shared<RaySphere>(glm::vec3(0, -100.5, -1), 100.0f, materialGround));
            _world.add(std::make_shared<RaySphere>(glm::vec3(0, 0, -1.2f), 0.5f, materialCenter));
            _world.add(std::make_shared<RaySphere>(glm::vec3(-1.0f, 0, -1.0f), 0.5f, materialLeft));
            _world.add(std::make_shared<RaySphere>(glm::vec3(1.0f, 0, -1.0f), 0.5f, materialRight));

            for (const auto& [_, e] : entities) {
                if (Light* light = dynamic_cast<Light*>(e.get())) {
                    Color color = light->getColor();
                    float intensity = light->getIntensity();
                    Transform& transform = light->getTransform();
                    
                    if (dynamic_cast<DirectionalLight*>(e.get()))
                        _lights.add(std::make_shared<RayDirectionalLight>(color, intensity, transform));
                    if (dynamic_cast<PointLight*>(e.get()))
                        _lights.add(std::make_shared<RayPointLight>(color, intensity, transform));
                    if (SpotLight* spotLight = dynamic_cast<SpotLight*>(e.get()))
                        _lights.add(std::make_shared<RaySpotLight>(color, intensity, transform, spotLight->getSize(), spotLight->getBlend()));
                }
            }

            _camera.aspectRatio() = 16.0 / 9.0;
            _camera.imageWidth() = 100;
            _camera.samplesPerPixel() = 100;
            _camera.maxDepth() = 50;
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