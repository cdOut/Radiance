#ifndef SHADOW_H
#define SHADOW_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stb_image_write.h>

class ShadowAtlas {
    public:
        ShadowAtlas() {
            glGenFramebuffers(1, &_depthMapFBO);
            glGenTextures(1, &_depthAtlas);

            glBindTexture(GL_TEXTURE_2D, _depthAtlas);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, _atlasSize, _atlasSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthAtlas, 0);

            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~ShadowAtlas() {
            if (_depthMapFBO) glDeleteFramebuffers(1, &_depthMapFBO);
            if (_depthAtlas) glDeleteTextures(1, &_depthAtlas);
        }

        glm::ivec4 getViewport(int lightIndex) const {
            int tileSize = _atlasSize / 8;

            int x = (lightIndex % 8) * tileSize;
            int y = (lightIndex / 8) * tileSize;

            return glm::ivec4(x, y, tileSize, tileSize);
        }

        unsigned int getDepthMapFBO() const {
            return _depthMapFBO;
        }

        unsigned int getDepthAtlas() const {
            return _depthAtlas;
        }

        void saveShadowAtlas(const char* filename) {
            if (!_depthAtlas) return;

            glBindTexture(GL_TEXTURE_2D, _depthAtlas);

            float* depthData = new float[_atlasSize * _atlasSize];
            glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthData);

            unsigned char* imageData = new unsigned char[_atlasSize * _atlasSize];
            for (int i = 0; i < _atlasSize * _atlasSize; i++) {
                float d = depthData[i];
                imageData[i] = (unsigned char)(255.0f * d);  
            }

            stbi_write_png(filename, _atlasSize, _atlasSize, 1, imageData, _atlasSize);

            delete[] depthData;
            delete[] imageData;
        }
    private:
        const int _atlasSize = 4096;

        unsigned int _depthMapFBO;
        unsigned int _depthAtlas;
};

class PointShadowAtlas {
    public:
        PointShadowAtlas() {
            glGenFramebuffers(1, &_depthMapFBO);
            glGenTextures(1, &_depthAtlas);

            glBindTexture(GL_TEXTURE_2D, _depthAtlas);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, _atlasSize, _atlasSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthAtlas, 0);

            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        ~PointShadowAtlas() {
            if (_depthMapFBO) glDeleteFramebuffers(1, &_depthMapFBO);
            if (_depthAtlas) glDeleteTextures(1, &_depthAtlas);
        }

        glm::ivec4 getViewport(int lightIndex, int face) const {
            int tileSize = _atlasSize / 8;
            int tileIndex = lightIndex * 6 + face;

            int x = (tileIndex % 8) * tileSize;
            int y = (tileIndex / 8) * tileSize;

            return glm::ivec4(x, y, tileSize, tileSize);
        }

        unsigned int getDepthMapFBO() const {
            return _depthMapFBO;
        }

        unsigned int getDepthAtlas() const {
            return _depthAtlas;
        }

        void saveShadowAtlas(const char* filename) {
            if (!_depthAtlas) return;

            glBindTexture(GL_TEXTURE_2D, _depthAtlas);

            float* depthData = new float[_atlasSize * _atlasSize];
            glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depthData);

            unsigned char* imageData = new unsigned char[_atlasSize * _atlasSize];
            for (int i = 0; i < _atlasSize * _atlasSize; i++) {
                float d = depthData[i];
                imageData[i] = (unsigned char)(255.0f * d);  
            }

            stbi_write_png(filename, _atlasSize, _atlasSize, 1, imageData, _atlasSize);

            delete[] depthData;
            delete[] imageData;
        }
    private:
        const int _atlasSize = 4096;
        const int _faces = 6;

        unsigned int _depthMapFBO;
        unsigned int _depthAtlas;
};

#endif