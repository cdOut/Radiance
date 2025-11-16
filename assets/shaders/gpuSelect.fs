#version 330 core

out vec4 FragColor;

uniform vec3 idColor;

void main() {
   FragColor = vec4(idColor / 255.0, 1.0);
}