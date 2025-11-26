#ifndef RAYCAMERA_H
#define RAYCAMERA_H

#include "Hittable.h"

class RayCamera {
    public:
        std::vector<unsigned char> render(const Hittable& world) {
            initialize();

            for (int j = 0; j < _imageHeight; j++) {
                for (int i = 0; i < _imageWidth; i++) {
                    auto pixelCenter = _pixel00Loc + (float(i) * _pixelDeltaU) + (float(_imageHeight - 1 - j) * _pixelDeltaV);
                    auto rayDirection = pixelCenter - _center;
                    Ray ray(_center, rayDirection);

                    Color pixelColor = rayColor(ray, world);

                    int index = (j * _imageWidth + i) * 3;
                    _imageData[index] = static_cast<unsigned char>(255.999 * pixelColor.x);
                    _imageData[index + 1] = static_cast<unsigned char>(255.999 * pixelColor.y);
                    _imageData[index + 2] = static_cast<unsigned char>(255.999 * pixelColor.z);
                }
            }

            return _imageData;
        }

        double& aspectRatio() { return _aspectRatio; }
        int& imageWidth() { return _imageWidth; }
    private:
        double _aspectRatio = 1.0;
        int _imageWidth = 100;
        int _imageHeight;
        glm::vec3 _center;
        glm::vec3 _pixel00Loc;
        glm::vec3 _pixelDeltaU;
        glm::vec3 _pixelDeltaV;
        std::vector<unsigned char> _imageData;

        void initialize() {
            _imageHeight = int(_imageWidth / _aspectRatio);
            _imageHeight = (_imageHeight < 1) ? 1 : _imageHeight;

            _imageData.resize(_imageWidth * _imageHeight * 3);

            _center = glm::vec3(0, 0, 0);

            float focalLength = 1.0;
            float viewportHeight = 2.0;
            float viewportWidth = viewportHeight * (float(_imageWidth) / _imageHeight);

            glm::vec3 viewportU(viewportWidth, 0, 0);
            glm::vec3 viewportV(0, -viewportHeight, 0);

            _pixelDeltaU = viewportU / float(_imageWidth);
            _pixelDeltaV = viewportV / float(_imageHeight);

            glm::vec3 viewportUpperLeft = _center - glm::vec3(0, 0, focalLength) - viewportU / 2.0f - viewportV / 2.0f;
            _pixel00Loc = viewportUpperLeft + 0.5f * (_pixelDeltaU + _pixelDeltaV);
        }

        Color rayColor(const Ray& ray, const Hittable& world) const {
            HitRecord rec;
            if (world.hit(ray, Interval(0, infinity), rec)) {
                return 0.5f * (rec.normal + Color(1.0f, 1.0f, 1.0f));
            }

            glm::vec3 unitDirection = glm::normalize(ray.direction());
            float a = 0.5 * unitDirection.y + 1.0;
            return (1.0f - a) * Color(1.0, 1.0, 1.0) + a * Color(0.5, 0.7, 1.0);
        }
};

#endif