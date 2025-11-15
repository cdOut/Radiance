#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "core/Shader.h"

class Billboard {
    public:
        Billboard() {
            float size = 1.0f;
            float h = size * 0.5f;

            std::vector<glm::vec3> v = {
                {-h, -h, 0.0f},
                { h, -h, 0.0f},
                {-h,  h, 0.0f},
                { h,  h, 0.0f},
            };

            std::vector <glm::vec2> txc {
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {0.0f, 1.0f},
                {1.0f, 1.0f},
            };

            for (size_t i = 0; i < v.size(); i++) {
                _vertices.insert(_vertices.end(), {v[i].x, v[i].y, v[i].z, txc[i].x, txc[i].y});
            }

            _indices = {
                0, 1, 2,
                2, 1, 3,
            };

            initializeBuffers();
        }

        ~Billboard() {
            glDeleteVertexArrays(1, &_VAO);
            glDeleteBuffers(1, &_VBO);
            glDeleteBuffers(1, &_EBO);
        }

        void setTexture(unsigned int texture) {
            _texture = texture;
        }

        void render(glm::mat4 model) {
            if (!_shader) return;

            _shader->use();

            _shader->setMat4("model", model);

            glDepthMask(GL_FALSE);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _texture);

            glBindVertexArray(_VAO);
            glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
        }

        void setShader(Shader* shader) { _shader = shader; }
    private:
        unsigned int _VAO, _VBO, _EBO;
        float _floatsPerVert = 5;
        std::vector<float> _vertices;
        std::vector<unsigned int> _indices;
        unsigned int _texture;

        Shader* _shader = nullptr;
        
        void initializeBuffers() {
            glGenVertexArrays(1, &_VAO);
            glGenBuffers(1, &_VBO);

            glBindVertexArray(_VAO);

            glBindBuffer(GL_ARRAY_BUFFER, _VBO);
            glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), _vertices.data(), GL_STATIC_DRAW);

            glGenBuffers(1, &_EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned int), _indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
        }
};

#endif