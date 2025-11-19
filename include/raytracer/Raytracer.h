#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <stb_image_write.h>
#include <iostream>

#include <glad/glad.h>

#include "Ray.h"

using Color = glm::vec3;

class Raytracer {
    public:
        static std::vector<unsigned char> raytrace() {
            auto aspectRatio = 16.0 / 9.0;
            int imageWidth = 1920;

            int imageHeight = int(imageWidth / aspectRatio);
            imageHeight = (imageHeight < 1) ? 1 : imageHeight;

            std::vector<unsigned char> imageData(imageWidth * imageHeight * 3);

            auto focalLength = 1.0;
            auto viewportHeight = 2.0;
            auto viewportWidth = viewportHeight * (double(imageWidth) / imageHeight);
            auto cameraCenter = glm::vec3(0, 0, 0);

            glm::vec3 viewportU(viewportWidth, 0, 0);
            glm::vec3 viewportV(0, -viewportHeight, 0);

            auto pixelDeltaU = viewportU / float(imageWidth);
            auto pixelDeltaV = viewportV / float(imageHeight);

            auto viewportUpperLeft = cameraCenter - glm::vec3(0, 0, focalLength) - viewportU / 2.0f - viewportV / 2.0f;
            auto pixel00Loc = viewportUpperLeft + 0.5f * (pixelDeltaU + pixelDeltaV);

            // Render
            for (int j = 0; j < imageHeight; j++) {
                std::clog << "\rScanlines remaining: " << (imageHeight - j) << ' ' << std::flush;
                for (int i = 0; i < imageWidth; i++) {
                    auto pixelCenter = pixel00Loc + (float(i) * pixelDeltaU) + (float(j) * pixelDeltaV);
                    auto rayDirection = pixelCenter - cameraCenter;
                    Ray ray(cameraCenter, rayDirection);

                    Color pixelColor = rayColor(ray);

                    int index = (j * imageWidth + i) * 3;
                    imageData[index] = static_cast<unsigned char>(255.999 * pixelColor.x);
                    imageData[index + 1] = static_cast<unsigned char>(255.999 * pixelColor.y);
                    imageData[index + 2] = static_cast<unsigned char>(255.999 * pixelColor.z);
                }
            }

            std::clog << "\rDone.                 \n";

            return imageData;
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
    private:
        static float hitSphere(const glm::vec3& center, float radius, const Ray& ray) {
            glm::vec3 oc = center - ray.origin();
            auto a = glm::dot(ray.direction(), ray.direction());
            auto b = -2.0 * glm::dot(ray.direction(), oc);
            auto c = glm::dot(oc, oc) - radius * radius;
            auto discriminant = b * b - 4 * a * c;
            
            if (discriminant < 0) {
                return -1.0;
            } else {
                return (-b - std::sqrt(discriminant)) / (2.0 * a);
            }
        }

        static Color rayColor(const Ray& ray) {
            auto t = hitSphere(glm::vec3(0, 0, -1), 0.5, ray);
            if (t > 0.0) {
                glm::vec3 N = glm::normalize(ray.at(t) - glm::vec3(0, 0, -1));
                return 0.5f * Color(N.x + 1, N.y + 1, N.z + 1);
            }

            glm::vec3 unitDirection = glm::normalize(ray.direction());
            float a = 0.5 * unitDirection.y + 1.0;
            return (1.0f - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
        }
};

#endif