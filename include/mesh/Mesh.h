#ifndef MESH_H
#define MESH_H

#define GLM_ENABLE_EXPERIMENTAL

#include <cmath>
#include <vector>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Entity.h"
#include "Shader.h"

class Mesh : public Entity {
    public:
        template<typename T>
        static std::unique_ptr<T> Create() {
            static_assert(std::is_base_of<Mesh, T>::value, "T must derive from Mesh");
            auto mesh = std::make_unique<T>();
            mesh->generateMesh();
            mesh->initializeBuffers();
            return mesh;
        }

        Mesh() : _VAO(0), _VBO(0), _EBO(0), _floatsPerVert(6) {
            _vertices.clear();
            _indices.clear();
        }

        virtual ~Mesh() {
            if (_VAO) glDeleteVertexArrays(1, &_VAO);
            if (_VBO) glDeleteBuffers(1, &_VBO);
            if (_EBO) glDeleteBuffers(1, &_EBO);
        }

        virtual void render(Shader& shader) const {
            shader.use();

            glm::mat4 model = getModelMatrix();
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

            shader.setMat4("model", model);
            shader.setMat3("normalMatrix", normalMatrix);

            glBindVertexArray(_VAO);
            if (!_indices.empty()) {
                glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_INT, 0);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, _vertices.size() / _floatsPerVert);
            }
            glBindVertexArray(0);
        }

    protected:
        unsigned int _VAO, _VBO, _EBO;
        unsigned int _floatsPerVert;
        std::vector<float> _vertices;
        std::vector<unsigned int> _indices;

        virtual void generateMesh() = 0;

        virtual void initializeBuffers() {
            glGenVertexArrays(1, &_VAO);
            glGenBuffers(1, &_VBO);

            glBindVertexArray(_VAO);

            glBindBuffer(GL_ARRAY_BUFFER, _VBO);
            glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), _vertices.data(), GL_STATIC_DRAW);

            if (!_indices.empty()) {
                glGenBuffers(1, &_EBO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _EBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned int), _indices.data(), GL_STATIC_DRAW);
            }

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
        }
};

#endif