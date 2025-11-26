#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <stb_image_write.h>

#include "RaytracerUtils.h"
#include "RayCamera.h"
#include "HittableList.h"
#include "RaySphere.h"

class Raytracer {
    public:
        static std::vector<unsigned char> raytrace() {
            HittableList world;
            world.add(std::make_shared<RaySphere>(glm::vec3(0, 0, -1), 0.5f));
            world.add(std::make_shared<RaySphere>(glm::vec3(0, -100.5, -1), 100.0f));

            RayCamera camera;
            camera.aspectRatio() = 16.0 / 9.0;
            camera.imageWidth() = 1920;

            return camera.render(world);
        }

        static unsigned int uploadRender(const unsigned char* data, int width, int height) {
            static unsigned int renderId = 0;

            if (!renderId)
                glGenTextures(1, &renderId);

            glBindTexture(GL_TEXTURE_2D, renderId);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);

            return renderId;
        }
};

#endif