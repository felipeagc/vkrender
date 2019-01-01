#version 450

#define MAX_LIGHTS 20
const float PI = 3.14159265359;

layout (location = 0) in vec2 texCoords;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 normal;

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
  vec4 pos;
} camera;

layout (push_constant) uniform MaterialPushConstant {
  vec4 baseColor;
  float metallic;
  float roughness;
  vec4 emissive;
  float hasNormalTexture;
} material;

layout (set = 1, binding = 0) uniform sampler2D albedoTexture;
layout (set = 1, binding = 1) uniform sampler2D normalTexture;
layout (set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout (set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout (set = 1, binding = 4) uniform sampler2D emissiveTexture;

struct Light {
  vec4 pos;
  vec4 color;
};

layout (set = 4, binding = 0) uniform EnvironmentUniform {
  vec3 sunDirection;
  float exposure;
  vec3 sunColor;
  float sunIntensity;
  uint lightCount;
  layout(offset = 48) Light lights[MAX_LIGHTS];
} environment;

layout (set = 4, binding = 2) uniform samplerCube irradianceMap;
layout (set = 4, binding = 3) uniform samplerCube radianceMap;
layout (set = 4, binding = 4) uniform sampler2D brdfLut;

layout (location = 0) out vec4 outColor;

const float MAX_REFLECTION_LOD = 9.0;

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

float geometrySchlickSmithGGX(float NdotL, float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = NdotL / (NdotL * (1.0 - k) + k);
	float GV = NdotV / (NdotV * (1.0 - k) + k);
	return GL * GV;
}

void main() {
  vec3 N = normalize(normal);
  vec3 V = normalize(camera.pos.xyz - worldPos);
  vec3 R = -normalize(reflect(V, N));

  vec4 albedoColor = texture(albedoTexture, texCoords);

  vec3 albedo = pow(albedoColor.rgb, vec3(2.2));
  vec4 metallicRoughness = texture(metallicRoughnessTexture, texCoords);
  float metallic = material.metallic * metallicRoughness.b;
  float roughness = material.roughness * metallicRoughness.g;

  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, albedo, metallic);

  vec3 Lo = vec3(0.0);

  // Directional light (sun)
  {
    vec3 L = normalize(-environment.sunDirection);
    vec3 H = normalize(V + L);
    vec3 radiance = environment.sunColor * environment.sunIntensity;

    float NdotL = max(dot(N, L), 0.0);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySchlickSmithGGX(NdotL, max(dot(N, V), 0.0), roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  // Point lights
  for (int i = 0; i < environment.lightCount; i++) {
    // Calculate per-light radiance
    vec3 lightPos = environment.lights[i].pos.xyz;
    vec3 L = normalize(lightPos - worldPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos - worldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = environment.lights[i].color.xyz * attenuation;

    float NdotL = max(dot(N, L), 0.0);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySchlickSmithGGX(NdotL, max(dot(N, V), 0.0), roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = pow(texture(irradianceMap, N).rgb, vec3(2.2));
  vec3 diffuse = irradiance * albedo;

  vec3 prefilteredColor = pow(textureLod(radianceMap, R, roughness * MAX_REFLECTION_LOD).rgb, vec3(2.2));
  vec2 brdf = pow(texture(brdfLut, vec2(max(dot(N, V), 0.0), 1.0 - roughness)).rg, vec2(2.2));
  vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

  vec3 ambient = kD * diffuse + specular;

  vec3 occlusion = texture(occlusionTexture, texCoords).rgb;
  vec3 emissive = texture(emissiveTexture, texCoords).rgb;

  vec3 color = (ambient + Lo) * occlusion.r;


  // HDR tonemapping
  color = vec3(1.0) - exp(-color * environment.exposure);

  // Gamma correct
  color = pow(color, vec3(1.0/2.2));

  color += emissive * material.emissive.xyz;

  outColor = vec4(color, albedoColor.a) * material.baseColor;
}
