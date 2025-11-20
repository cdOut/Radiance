#version 330 core

in vec3 FragPos;

uniform vec3 lightPosition;
uniform float farPlane;
uniform bool isPointLight;

void main() {
   if (isPointLight) {
      gl_FragDepth = length(FragPos - lightPosition) / farPlane;
   } else {
      gl_FragDepth = gl_FragCoord.z;
   }
}