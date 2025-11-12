#version 330 core

in vec3 FragPos;
out vec4 FragColor;

uniform mat4 view;

uniform float fadeStart = 5.0;
uniform float fadeEnd   = 20.0;

void main() {
    vec3 camPos = vec3(inverse(view)[3]);
    float dist = distance(camPos, FragPos);
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);

    vec3 color;
    if (abs(FragPos.x) < 0.01)
        color = vec3(0.35, 0.55, 0.95);
    else if (abs(FragPos.z) < 0.01)
        color = vec3(0.95, 0.35, 0.35);
    else
        color = vec3(0.75);

    FragColor = vec4(color, 0.5 * fade);
}
