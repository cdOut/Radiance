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
        Mesh() : VAO(0), VBO(0), EBO(0), color(1.0f) {}

        virtual ~Mesh() {
            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
        }

        virtual void render(Shader& shader) const {
            shader.use();

            int modelLoc = glGetUniformLocation(shader.ID, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform.getModelMatrix()));

            int colorLoc = glGetUniformLocation(shader.ID, "color");
            glUniform3fv(colorLoc, 1, glm::value_ptr(color));

            glBindVertexArray(VAO);
            if (EBO != 0) {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, vertices.size() / floatsPerVert);
            }
            glBindVertexArray(0);
        }

        Transform& getTransform() { return transform; }
    protected:
        std::vector<float> vertices;
        unsigned int floatsPerVert;
        std::vector<unsigned int> indices;
        unsigned int VAO, VBO, EBO;

        Transform transform;
        glm::vec3 color;
};

#endif