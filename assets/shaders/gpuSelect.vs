#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int isBillboard;

void main() {
    if (isBillboard == 1) {
        mat3 camera = mat3(view);

        vec3 right = vec3(camera[0][0], camera[1][0], camera[2][0]);
        vec3 up = vec3(camera[0][1], camera[1][1], camera[2][1]);

        vec3 center = vec3(model[3]);

        float scalex = length(vec3(model[0]));
        float scaley = length(vec3(model[1]));

        vec3 pos = center + right * (aPos.x * scalex) + up * (aPos.y * scaley);

        gl_Position = projection * view * vec4(pos, 1.0);
        FragPos = vec3(model * vec4(pos, 1.0));
    } else {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        FragPos = vec3(model * vec4(aPos, 1.0));
    }
}