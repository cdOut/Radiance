#ifndef MESH_H
#define MESH_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Shader.h"

struct Transform {
    glm::vec3 position {0.0f};
    glm::vec3 rotation {0.0f};
    glm::vec3 scale {1.0f};

    glm::mat4 getModelMatrix() const {
        glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), position);
        glm::quat rotationQuat = glm::quat(glm::radians(rotation));
        glm::mat4 rotationMat = glm::toMat4(rotationQuat);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
        return translationMat * rotationMat * scaleMat;
    }
};

class Mesh {
    public:
        Mesh() : VAO(0), VBO(0), EBO(0), floatsPerVert(3), color(1.0f) {}

        virtual ~Mesh() {
            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
        }

        template<typename T>
        static std::unique_ptr<T> Create() {
            static_assert(std::is_base_of<Mesh, T>::value, "T must derive from Mesh");
            auto mesh = std::make_unique<T>();
            mesh->generateMesh();
            mesh->initializeBuffers();
            return mesh;
        }

        virtual void render(Shader& shader) const {
            shader.use();

            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(transform.getModelMatrix()));
            glUniform3fv(glGetUniformLocation(shader.ID, "color"), 1, glm::value_ptr(color));

            glBindVertexArray(VAO);
            if (!indices.empty()) {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, vertices.size() / floatsPerVert);
            }
            glBindVertexArray(0);
        }
    protected:
        unsigned int VAO, VBO, EBO;
        unsigned int floatsPerVert;
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        Transform transform;
        glm::vec3 color;

        virtual void generateMesh() = 0;

        virtual void initializeBuffers() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            if (!indices.empty()) {
                glGenBuffers(1, &EBO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
            }

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }
};

#endif