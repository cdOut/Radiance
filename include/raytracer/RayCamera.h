#ifndef RAYCAMERA_H
#define RAYCAMERA_H

#include "Hittable.h"

class RayCamera {
    public:
        std::vector<unsigned char> render(const Hittable& world) {
            initialize();

            for (int j = 0; j < _imageHeight; j++) {
                std::clog << "\rScanlines remaining: " << (_imageHeight - j) << ' ' << std::flush;
                for (int i = 0; i < _imageWidth; i++) {
                    Color pixelColor(0.0f, 0.0f, 0.0f);
                    for (int sample = 0; sample < _samplesPerPixel; sample++) {
                        Ray ray = getRay(i, j);
                        pixelColor += rayColor(ray, world);
                    }
                    pixelColor *= _pixelSamplesScale;

                    int index = (j * _imageWidth + i) * 3;
                    static const Interval intensity(0.0f, 0.999f);
                    _imageData[index] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.x));
                    _imageData[index + 1] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.y));
                    _imageData[index + 2] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.z));
                }
            }
            std::clog << "\rDone.                 \n";

            return _imageData;
        }

        float& aspectRatio() { return _aspectRatio; }
        int& imageWidth() { return _imageWidth; }
        int& imageHeight() { return _imageHeight; }
        int& samplesPerPixel() { return _samplesPerPixel; }
    private:
        float _aspectRatio = 1.0;
        int _imageWidth = 100;
        int _samplesPerPixel = 10;

        int _imageHeight;
        float _pixelSamplesScale;
        glm::vec3 _center;
        glm::vec3 _pixel00Loc;
        glm::vec3 _pixelDeltaU;
        glm::vec3 _pixelDeltaV;
        std::vector<unsigned char> _imageData;

        void initialize() {
            _imageHeight = int(_imageWidth / _aspectRatio);
            _imageHeight = (_imageHeight < 1) ? 1 : _imageHeight;

            _imageData.resize(_imageWidth * _imageHeight * 3);

            _pixelSamplesScale = 1.0f / float(_samplesPerPixel);

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

        Ray getRay(int i, int j) const {
            glm::vec3 offset = sampleSquare();
            glm::vec3 pixelSample = _pixel00Loc + (float(i) + offset.x) * _pixelDeltaU + (float(_imageHeight - 1 - j) + offset.y) * _pixelDeltaV;

            glm::vec3 rayOrigin = _center;
            glm::vec3 rayDirection = pixelSample - rayOrigin;

            return Ray(rayOrigin, rayDirection);
        }

        glm::vec3 sampleSquare() const {
            float r1 = randomFloat();
            float r2 = randomFloat();
            return glm::vec3(r1 - 0.5f, r2 - 0.5f, 0);
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