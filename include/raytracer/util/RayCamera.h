#ifndef RAYCAMERA_H
#define RAYCAMERA_H

#include "../hittable/Hittable.h"
#include "RayMaterial.h"
#include "../light/RayLight.h"
#include "../light/RayLightList.h"

class RayCamera {
    public:
        std::function<void(int j)> onScanlineFinished;
        unsigned char* imageDataBuffer;

        void render(const Hittable& world, const RayLightList& lights) {
            initialize();

            RayCamera::scanlineSize.store(_imageHeight);
            for (int j = _imageHeight - 1; j >= 0; j--) {
                RayCamera::currentScanline.store(j);
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
                    imageDataBuffer[index] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.x));
                    imageDataBuffer[index + 1] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.y));
                    imageDataBuffer[index + 2] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.z));
                }

                if (onScanlineFinished) onScanlineFinished(j);
            }
        }

        float& aspectRatio() { return _aspectRatio; }
        int& imageWidth() { return _imageWidth; }
        int& imageHeight() { return _imageHeight; }
        int& samplesPerPixel() { return _samplesPerPixel; }
        int& maxDepth() { return _maxDepth; }
        Color& skyboxColor() { return _skyboxColor; }
        Transform& transform() { return _transform; }

        inline static std::atomic<int> currentScanline = -1;
        inline static std::atomic<int> scanlineSize = -1;
    private:
        float _aspectRatio = 1.0;
        int _imageWidth = 100;
        int _samplesPerPixel = 10;
        int _maxDepth = 10;
        Color _skyboxColor = Color(0.0f);

        double fov = 90.0f;
        int _imageHeight;
        float _pixelSamplesScale;
        glm::vec3 _center;
        glm::vec3 _pixel00Loc;
        glm::vec3 _pixelDeltaU;
        glm::vec3 _pixelDeltaV;
        std::vector<unsigned char> _imageData;

        Transform _transform;
        glm::vec3 _forward {0.0f, 0.0f, -1.0f};
        glm::vec3 _right {1.0f, 0.0f, 0.0f};
        glm::vec3 _up {0.0f, 1.0f, 0.0f};

        void calculateVectors() {
            glm::vec3 direction;
            direction.x = cosf(glm::radians(_transform.rotation.y)) * cosf(glm::radians(_transform.rotation.x));
            direction.y = sinf(glm::radians(_transform.rotation.x));
            direction.z = sinf(glm::radians(_transform.rotation.y)) * cosf(glm::radians(_transform.rotation.x));

            _forward = glm::normalize(direction);
            _right = glm::normalize(glm::cross(_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            _up = glm::normalize(glm::cross(_right, _forward));
        }

        void initialize() {
            _imageHeight = int(_imageWidth / _aspectRatio);
            _imageHeight = (_imageHeight < 1) ? 1 : _imageHeight;

            _imageData.resize(_imageWidth * _imageHeight * 3);

            _pixelSamplesScale = 1.0f / float(_samplesPerPixel);

            _center = _transform.position;
            calculateVectors();

            float focalLength = 1.0;
            float theta = glm::radians(fov);
            float h = glm::tan(theta / 2.0f);
            float viewportHeight = 2.0 * h * focalLength;
            float viewportWidth = viewportHeight * (float(_imageWidth) / _imageHeight);

            glm::vec3 viewportU = _right * viewportWidth;
            glm::vec3 viewportV = -_up * viewportHeight;

            _pixelDeltaU = viewportU / float(_imageWidth);
            _pixelDeltaV = viewportV / float(_imageHeight);

            glm::vec3 viewportUpperLeft = _center + _forward * focalLength - viewportU / 2.0f - viewportV / 2.0f;
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
            if (world.raymarch(ray, rec)) {
                Color resultColor(0.0f);

                for (const auto& lightPtr : lights.lights) {
                    const RayLight& light = *lightPtr;
                    glm::vec3 lightDir = glm::normalize(light.directionFrom(rec.point));

                    Ray shadowRay(rec.point + rec.normal * 0.001f, lightDir);
                    HitRecord shadowRec;
                    if (!world.raymarch(shadowRay, shadowRec)) {
                        resultColor += rec.material->shade(ray, rec, lightDir, light);
                    }
                }

                Ray scattered;
                Color attenuation;
                if (rec.material->scatter(ray, rec, attenuation, scattered)) {
                    resultColor += attenuation * rayColor(scattered, depth - 1, world, lights);
                }

                return resultColor;
            }

            return _skyboxColor;
        }
};

#endif