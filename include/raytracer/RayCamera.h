#ifndef RAYCAMERA_H
#define RAYCAMERA_H

#include "Hittable.h"
#include "RayMaterial.h"
#include "RayLight.h"
#include "RayLightList.h"

class RayCamera {
    public:
        std::vector<unsigned char> render(const Hittable& world, const RayLightList& lights) {
            initialize();

            for (int j = 0; j < _imageHeight; j++) {
                std::clog << "\rScanlines remaining: " << (_imageHeight - j) << ' ' << std::flush;
                for (int i = 0; i < _imageWidth; i++) {
                    Color pixelColor(0.0f, 0.0f, 0.0f);
                    for (int sample = 0; sample < _samplesPerPixel; sample++) {
                        Ray ray = getRay(i, j);
                        pixelColor += rayColor(ray, _maxDepth, world, lights);
                    }
                    pixelColor *= _pixelSamplesScale;

                    pixelColor.x = linearToGamma(pixelColor.x);
                    pixelColor.y = linearToGamma(pixelColor.y);
                    pixelColor.z = linearToGamma(pixelColor.z);

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
        int& maxDepth() { return _maxDepth; }
    private:
        float _aspectRatio = 1.0;
        int _imageWidth = 100;
        int _samplesPerPixel = 10;
        int _maxDepth = 10;

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

        Color rayColor(const Ray& ray, int depth, const Hittable& world, const RayLightList& lights) const {
            if (depth <= 0)
                return Color(0.0f, 0.0f, 0.0f);

            HitRecord rec;
            if (world.hit(ray, Interval(0.001, infinity), rec)) {
                Color resultColor(0.0f);

                for (const auto& lightPtr : lights.lights) {
                    const RayLight& light = *lightPtr;
                    glm::vec3 lightDir = glm::normalize(light.directionFrom(rec.point));

                    Ray shadowRay(rec.point + rec.normal * 0.001f, lightDir);
                    HitRecord shadowRec;
                    if (!world.hit(shadowRay, Interval(0.001, infinity), shadowRec)) {
                        float nDotL = glm::max(glm::dot(rec.normal, lightDir), 0.0f);
                        resultColor += rec.material->albedo() * light.intensityAt(rec.point) * nDotL;
                    }
                }

                Ray scattered;
                Color attenuation;
                if (rec.material->scatter(ray, rec, attenuation, scattered)) {
                    resultColor += attenuation * rayColor(scattered, depth - 1, world, lights);
                }

                return resultColor;
            }

            return Color(0.0f, 0.0f, 0.0f);
        }
};

#endif