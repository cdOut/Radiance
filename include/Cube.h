#ifndef CUBE_H
#define CUBE_H

#include "Mesh.h"

class Cube : public Mesh {
    public:
        Cube() {
            floatsPerVert = 3;

            for (int x = 0; x <= 1; x++) {
                for (int y = 0; y <= 1; y++) {
                    for (int z = 0; z <= 1; z++) {
                        vertices.insert(vertices.end(), {x - 0.5f, y - 0.5f, z - 0.5f});
                    }
                }
            }

            indices = {
                0, 1, 3,
                0, 3, 2,
                4, 6, 7,
                4, 7, 5,
                4, 0, 2,
                4, 2, 6,
                1, 5, 7,
                1, 7, 3,
                2, 3, 7,
                2, 7, 6,
                4, 5, 1,
                4, 1, 0
            };

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }
};

#endif