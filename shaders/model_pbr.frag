#version 450

#define MAX_LIGHTS 20
const float PI = 3.14159265359;

layout (location = 0) in vec2 texCoords;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 normal;

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera;

layout (set = 1, binding = 0) uniform MaterialUniform {
  vec3 albedo;
  float dummy1;
  float metallic;
  float roughness;
  float ao;
} material;

layout (set = 1, binding = 1) uniform sampler2D albedoTexture;
layout (set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;

struct Light {
  vec3 pos;
  float dummy1;
  vec3 color;
  float dummy2;
};

layout(set = 3, binding = 0) uniform LightingUniform {
  Light lights[MAX_LIGHTS];
  uint lightCount;
} lighting;

layout (set = 4, binding = 1) uniform samplerCube irradianceMap;

layout (location = 0) out vec4 outColor;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r*r) / 8.0;

  float num = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);

  float ggx2 = geometrySchlickGGX(NdotV, roughness);
  float ggx1 = geometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

void main() {
  vec3 viewPos = vec3(camera.view[3][0], camera.view[3][1], camera.view[3][2]);

  vec3 N = normalize(normal);
  vec3 V = normalize(viewPos - worldPos);

  vec3 albedo = texture(albedoTexture, texCoords).rgb * material.albedo;
  vec2 metallicRoughness = texture(metallicRoughnessTexture, texCoords).bg;
  float metallic = material.metallic * metallicRoughness.y;
  float roughness = material.roughness * metallicRoughness.x;

  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, albedo, metallic);

  vec3 Lo = vec3(0.0);
  for (int i = 0; i < lighting.lightCount; i++) {
    vec3 L = normalize(lighting.lights[i].pos - worldPos);
    vec3 H = normalize(V + L);

    float distance = length(lighting.lights[i].pos - worldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lighting.lights[i].color * attenuation;

    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  // vec3 ambient = texture(irradianceMap, N).rgb;

  vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
  vec3 kD = 1.0 - kS;
  vec3 irradiance = texture(irradianceMap, N).rgb;
  vec3 diffuse = irradiance * albedo;
  vec3 ambient = kD * diffuse;

  // vec3 ambient = vec3(0.03) * albedo;
  vec3 color = ambient + Lo;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0/2.2));

  outColor = vec4(color, 1.0);
}
