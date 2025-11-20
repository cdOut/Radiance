#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

struct DirectionalLight {
    vec3 color;
    vec3 direction;
};

struct PointLight {
    vec3 color;
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    int atlasIndex;
    float farPlane;
};

struct SpotLight {
    vec3 color;
    vec3 position;
    vec3 direction;

    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

uniform vec3 viewPos;
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;

#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 32
#define MAX_SPOT_LIGHTS 8

uniform int directionalLightsAmount;
uniform int pointLightsAmount;
uniform int spotLightsAmount;

uniform DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

uniform sampler2D pointShadowAtlas;

#define PI 3.14159265359

float DistributionGGX(vec3 N, vec3 H, float a) {
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
	
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k) {
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return nom / denom;
}
  
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calculatePBR(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);                
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float calculatePointShadow(PointLight light, vec3 fragPos) {
    vec3 lightToFrag = fragPos - light.position;
    float currentDepth = length(lightToFrag);

    vec3 L = normalize(lightToFrag);
    int face;
    float absX = abs(L.x), absY = abs(L.y), absZ = abs(L.z);

    if (absX > absY && absX > absZ)
        face = L.x > 0 ? 0 : 1;
    else if (absY > absX && absY > absZ)
        face = L.y > 0 ? 2 : 3;
    else
        face = L.z > 0 ? 4 : 5;

    vec2 uv;
    if (face == 0) uv = vec2(-L.z, -L.y) / abs(L.x);
    else if (face == 1) uv = vec2( L.z, -L.y) / abs(L.x);
    else if (face == 2) uv = vec2( L.x,  L.z) / abs(L.y);
    else if (face == 3) uv = vec2( L.x, -L.z) / abs(L.y);
    else if (face == 4) uv = vec2( L.x, -L.y) / abs(L.z);
    else uv = vec2(-L.x, -L.y) / abs(L.z);

    uv = uv * 0.5 + 0.5;

    int tileIndex = light.atlasIndex + face;
    float tilesPerRow = 16.0;
    float tileSize = 1.0 / tilesPerRow;

    float x = float(tileIndex % int(tilesPerRow)) * tileSize;
    float y = float(tileIndex / int(tilesPerRow)) * tileSize;

    uv = vec2(x + uv.x * tileSize, y + uv.y * tileSize);

    float closestDepth = texture(pointShadowAtlas, uv).r;
    closestDepth *= light.farPlane;

    float bias = 0.05;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 Lo = vec3(0.0);

    // directional lights
    for (int i = 0; i < directionalLightsAmount; i++) {
        vec3 L = normalize(-directionalLights[i].direction);
        vec3 radiance = directionalLights[i].color;

        Lo += calculatePBR(N, V, L, radiance, albedo, metallic, roughness);
    }

    // point lights
    for (int i = 0; i < pointLightsAmount; i++) {
        vec3 L = normalize(pointLights[i].position - FragPos);

        float dist = length(pointLights[i].position - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * dist + pointLights[i].quadratic * dist * dist);

        vec3 radiance = pointLights[i].color * attenuation;

        float shadow = calculatePointShadow(pointLights[i], FragPos);

        Lo += (1.0 - shadow) * calculatePBR(N, V, L, radiance, albedo, metallic, roughness);
    }

    // spot lights
    for (int i = 0; i < spotLightsAmount; i++) {
        vec3 L = normalize(spotLights[i].position - FragPos);

        float dist = length(spotLights[i].position - FragPos);
        float attenuation = 1.0 / (spotLights[i].constant + spotLights[i].linear * dist + spotLights[i].quadratic * dist * dist);

        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float intensity = clamp((theta - spotLights[i].outerCutOff) / epsilon, 0.0, 1.0);

        vec3 radiance = spotLights[i].color * attenuation * intensity;

        Lo += calculatePBR(N, V, L, radiance, albedo, metallic, roughness);
    }

    vec3 color = Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}