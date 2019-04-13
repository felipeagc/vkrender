#version 450

#define MAX_LIGHTS 20
const float PI = 3.14159265359;

struct Light {
  vec4 pos;
  vec4 color;
};

layout (location = 0) in vec2 tex_coords;
layout (location = 1) in vec3 world_pos;
layout (location = 2) in vec3 normal;

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
  vec4 pos;
} camera;

layout (set = 1, binding = 0) uniform EnvironmentUniform {
  vec3 sun_direction;
  float exposure;
  vec3 sun_color;
  float sun_intensity;
  float radiance_mip_levels;
  uint light_count;
  layout(offset = 48) Light lights[MAX_LIGHTS];
} environment;

layout (set = 1, binding = 2) uniform samplerCube irradiance_map;
layout (set = 1, binding = 3) uniform samplerCube radiance_map;
layout (set = 1, binding = 4) uniform sampler2D brdf_lut;

layout (set = 4, binding = 0) uniform sampler2D albedo_texture;
layout (set = 4, binding = 1) uniform sampler2D normal_texture;
layout (set = 4, binding = 2) uniform sampler2D metallic_roughness_texture;
layout (set = 4, binding = 3) uniform sampler2D occlusion_texture;
layout (set = 4, binding = 4) uniform sampler2D emissive_texture;
layout (set = 4, binding = 5) uniform MaterialUniform {
  vec4 base_color;
  float metallic;
  float roughness;
  vec4 emissive;
  float has_normal_texture;
} material;

layout (location = 0) out vec4 out_color;

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

float distribution_ggx(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

float geometry_schlick_smith_ggx(float NdotL, float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = NdotL / (NdotL * (1.0 - k) + k);
	float GV = NdotV / (NdotV * (1.0 - k) + k);
	return GL * GV;
}

void main() {
  vec3 N = normalize(normal);
  vec3 V = normalize(camera.pos.xyz - world_pos);
  vec3 R = -normalize(reflect(V, N));

  vec4 albedo_color = texture(albedo_texture, tex_coords);

  vec3 albedo = pow(albedo_color.rgb, vec3(2.2));
  vec4 metallic_roughness = texture(metallic_roughness_texture, tex_coords);
  float metallic = material.metallic * metallic_roughness.b;
  float roughness = material.roughness * metallic_roughness.g;

  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, albedo, metallic);

  vec3 Lo = vec3(0.0);

  // Directional light (sun)
  {
    vec3 L = normalize(-environment.sun_direction);
    vec3 H = normalize(V + L);
    vec3 radiance = environment.sun_color * environment.sun_intensity;

    float NdotL = max(dot(N, L), 0.0);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_schlick_smith_ggx(NdotL, max(dot(N, V), 0.0), roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  // Point lights
  for (int i = 0; i < environment.light_count; i++) {
    // Calculate per-light radiance
    vec3 light_pos = environment.lights[i].pos.xyz;
    vec3 L = normalize(light_pos - world_pos);
    vec3 H = normalize(V + L);
    float distance = length(light_pos - world_pos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = environment.lights[i].color.xyz * attenuation;

    float NdotL = max(dot(N, L), 0.0);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_schlick_smith_ggx(NdotL, max(dot(N, V), 0.0), roughness);
    vec3 F = fresnel_schlick(max(dot(H, V), 0.0), F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = pow(texture(irradiance_map, N).rgb, vec3(2.2));
  vec3 diffuse = irradiance * albedo;

  vec3 prefiltered_color = pow(textureLod(radiance_map, R, roughness * environment.radiance_mip_levels).rgb, vec3(2.2));
  vec2 brdf = pow(texture(brdf_lut, vec2(max(dot(N, V), 0.0), 1.0 - roughness)).rg, vec2(2.2));
  vec3 specular = prefiltered_color * (F * brdf.x + brdf.y);

  vec3 ambient = kD * diffuse + specular;

  vec3 occlusion = texture(occlusion_texture, tex_coords).rgb;
  vec3 emissive = texture(emissive_texture, tex_coords).rgb;

  vec3 color = (ambient + Lo) * occlusion.r;

  // HDR tonemapping
  color = vec3(1.0) - exp(-color * environment.exposure);

  // Gamma correct
  color = pow(color, vec3(1.0/2.2));

  color += emissive * material.emissive.xyz;

  out_color = vec4(color, albedo_color.a) * material.base_color;
}
