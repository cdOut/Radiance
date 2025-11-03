#ifndef MESH_H
#define MESH_H

#define GLM_ENABLE_EXPERIMENTAL

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

        Mesh() : VAO(0), VBO(0), EBO(0), floatsPerVert(6), color(1.0f) {
            vertices.clear();
            indices.clear();
        }

        virtual ~Mesh() {
            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
        }

        virtual void render(Shader& shader) const {
            shader.use();

            glm::mat4 model = getModelMatrix();
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

            shader.setMat4("model", model);
            shader.setMat3("normalMatrix", normalMatrix);

            glBindVertexArray(VAO);
            if (!indices.empty()) {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, vertices.size() / floatsPerVert);
            }
            glBindVertexArray(0);
        }

        Transform& getTransform() { return transform; }
        glm::vec3& getColor() { return color; }
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

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) (3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);
        }
    private:
        glm::mat4 getModelMatrix() const {
            glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), transform.position);
            glm::quat rotationQuat = glm::quat(glm::yawPitchRoll(
                glm::radians(transform.rotation.y),
                glm::radians(transform.rotation.x),
                glm::radians(transform.rotation.z)
            ));
            glm::mat4 rotationMat = glm::toMat4(rotationQuat);
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), transform.scale);
            return translationMat * rotationMat * scaleMat;
        }
};

#endif