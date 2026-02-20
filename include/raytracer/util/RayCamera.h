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

            int numThreads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            std::atomic<int> nextRow(_imageHeight - 1);

            RayCamera::scanlineSize.store(_imageHeight);
            
            auto worker = [&]() {
                int j;
                while ((j = nextRow.fetch_sub(1)) >= 0) {
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
                        imageDataBuffer[index]     = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.x));
                        imageDataBuffer[index + 1] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.y));
                        imageDataBuffer[index + 2] = static_cast<unsigned char>(256 * intensity.clamp(pixelColor.z));
                    }
                    if (onScanlineFinished) onScanlineFinished(j);
                }
            };

            for (int i = 0; i < numThreads; i++)
                threads.emplace_back(worker);
            for (auto& t : threads)
                t.join();
            
            denoise();
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

        void denoise() {
            for (int pass = 0; pass < 3; pass++) {
                denoisePass();
            }
        }

        void denoisePass() {
            int height = _imageHeight;
            std::vector<unsigned char> output(_imageWidth * height * 3);

            const int radius = 2;
            const float sigmaSpace = 2.0f;

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < _imageWidth; x++) {
                    glm::vec3 centerColor = getPixel(x, y);
                    float brightness = (centerColor.r + centerColor.g + centerColor.b) / 3.0f;
                    float sigmaColor = glm::mix(0.05f, 0.25f, 1.0f - brightness);
                    glm::vec3 sum(0.0f);
                    float weightSum = 0.0f;

                    for (int dy = -radius; dy <= radius; dy++) {
                        for (int dx = -radius; dx <= radius; dx++) {
                            int nx = glm::clamp(x + dx, 0, _imageWidth - 1);
                            int ny = glm::clamp(y + dy, 0, height - 1);

                            glm::vec3 neighborColor = getPixel(nx, ny);

                            float spaceDist = float(dx * dx + dy * dy) / (2.0f * sigmaSpace * sigmaSpace);
                            glm::vec3 colorDiff = neighborColor - centerColor;
                            float colorDist = glm::dot(colorDiff, colorDiff) / (2.0f * sigmaColor * sigmaColor);

                            float weight = expf(-spaceDist - colorDist);
                            sum += neighborColor * weight;
                            weightSum += weight;
                        }
                    }

                    glm::vec3 result = sum / weightSum;
                    int index = (y * _imageWidth + x) * 3;
                    output[index] = static_cast<unsigned char>(glm::clamp(result.r * 255.0f, 0.0f, 255.0f));
                    output[index + 1] = static_cast<unsigned char>(glm::clamp(result.g * 255.0f, 0.0f, 255.0f));
                    output[index + 2] = static_cast<unsigned char>(glm::clamp(result.b * 255.0f, 0.0f, 255.0f));
                }
            }

            std::copy(output.begin(), output.end(), imageDataBuffer);
        }

        glm::vec3 getPixel(int x, int y) const {
            int index = (y * _imageWidth + x) * 3;
            return glm::vec3(
                imageDataBuffer[index]     / 255.0f,
                imageDataBuffer[index + 1] / 255.0f,
                imageDataBuffer[index + 2] / 255.0f
            );
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

                    float biasAmount = 0.1f;
                    Ray shadowRay(rec.point + rec.normal * biasAmount, lightDir);
                    HitRecord shadowRec;
                    bool inShadow = false;
                    if (world.raymarch(shadowRay, shadowRec)) {
                        float shadowDist = glm::length(shadowRec.point - shadowRay.origin());
                        float lightDist = light.distanceFrom(rec.point) - biasAmount;
                        if (glm::dot(shadowRec.normal, lightDir) < 0.0f && shadowDist < lightDist) {
                            inShadow = true;
                        }
                    }
                    if (!inShadow) {
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