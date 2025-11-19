#version 330 core

in vec4 FragPos;

uniform vec3 lightPosition;
uniform float farPlane;

void main() {
   gl_FragDepth = length(FragPos.xyz - lightPosition) / farPlane;
}