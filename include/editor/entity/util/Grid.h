#ifndef GRID_H
#define GRID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "../../Shader.h"
#include "../Entity.h"

class Grid : public Entity {
    public:
        Grid(int size = 50, float spacing = 1.0f) {
            int halfSize = size / 2;
            for (int i = -halfSize; i <= halfSize; i++) {
                float x = i * spacing;
                float z = i * spacing;

                _vertices.insert(_vertices.end(), { -halfSize * spacing, 0.0f, z });
                _vertices.insert(_vertices.end(), {  halfSize * spacing, 0.0f, z });

                _vertices.insert(_vertices.end(), { x, 0.0f, -halfSize * spacing });
                _vertices.insert(_vertices.end(), { x, 0.0f,  halfSize * spacing });
            }

            initializeBuffers();
        }

        ~Grid() {
            glDeleteVertexArrays(1, &_VAO);
            glDeleteBuffers(1, &_VBO);
        }

        void handleCameraPos(const glm::vec3& cameraPos) {
            _transform.position = glm::vec3(glm::floor(cameraPos.x), 0.0f, glm::floor(cameraPos.z));
        }

        void render() {
            if (!_shader) return;
            _shader->use();

            glm::mat4 model = getModelMatrix();

            _shader->setMat4("model", model);

            glDepthMask(GL_FALSE);

            glBindVertexArray(_VAO);
            glDrawArrays(GL_LINES, 0, _vertices.size() / _floatsPerVert);
            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
        }
    private:
        unsigned int _VAO, _VBO;
        float _floatsPerVert = 3;
        std::vector<float> _vertices;

        void initializeBuffers() {
            glGenVertexArrays(1, &_VAO);
            glGenBuffers(1, &_VBO);

            glBindVertexArray(_VAO);

            glBindBuffer(GL_ARRAY_BUFFER, _VBO);
            glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), _vertices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }
};

#endif