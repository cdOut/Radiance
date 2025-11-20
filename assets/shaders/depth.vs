#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 model;

uniform mat4 lightSpaceMatrix;
uniform mat4 shadowMatrix;
uniform bool isPointLight;

void main() {
    if (isPointLight) {
        FragPos = (model * vec4(aPos, 1.0)).xyz;
        gl_Position = shadowMatrix * model * vec4(aPos, 1.0);
    } else {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }
}