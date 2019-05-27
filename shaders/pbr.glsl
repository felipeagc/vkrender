#ifndef PBR_GLSL
#define PBR_GLSL

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

const float GAMMA = 2.2f;

vec4 srgb_to_linear(vec4 srgb_in) {
  vec3 lin_out = pow(srgb_in.xyz, vec3(GAMMA));
  return vec4(lin_out, srgb_in.w);
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

float distribution_ggx(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = clamp(dot(N, H), 0.0, 1.0);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / denom;
}

float geometry_schlick_smith_ggx(float NdotL, float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r*r) / 8.0;
  float GL = NdotL / (NdotL * (1.0 - k) + k);
  float GV = NdotV / (NdotV * (1.0 - k) + k);
  return GL * GV;
}

// Inputs and outputs are in linear color space
vec4 calculate_pbr(
    vec3 world_pos,
    vec4 albedo,
    float metallic,
    float roughness,
    float occlusion,
    vec3 emissive,
    vec3 N,
    vec3 V) {
  vec3 R = -normalize(reflect(V, N));

  float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);

  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, albedo.rgb, metallic);

  vec3 Lo = vec3(0.0);

  // Directional light (sun)
  {
    vec3 L = normalize(-environment.sun_direction);
    vec3 H = normalize(V + L);
    vec3 radiance = environment.sun_color * environment.sun_intensity;

    float NdotL = clamp(dot(N, L), 0.001, 1.0);
    float VdotH = clamp(dot(H, V), 0.0, 1.0);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_schlick_smith_ggx(NdotL, NdotV, roughness);
    vec3 F = fresnel_schlick(VdotH, F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
  }

  // Point lights
  for (int i = 0; i < environment.light_count; i++) {
    // Calculate per-light radiance
    vec3 light_pos = environment.lights[i].pos.xyz;
    vec3 L = normalize(light_pos - world_pos);
    vec3 H = normalize(V + L);
    float distance = length(light_pos - world_pos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = environment.lights[i].color.rgb * attenuation;

    float NdotL = clamp(dot(N, L), 0.001, 1.0);
    float VdotH = clamp(dot(H, V), 0.0, 1.0);

    // Cook-Torrance BRDF
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_schlick_smith_ggx(NdotL, NdotV, roughness);
    vec3 F = fresnel_schlick(VdotH, F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
  }

  vec3 F = fresnel_schlick_roughness(max(dot(N, V), 0.0), F0, roughness);

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = srgb_to_linear(texture(irradiance_map, N)).rgb;
  vec3 diffuse = irradiance * albedo.rgb;

  vec3 prefiltered_color = srgb_to_linear(
      textureLod(radiance_map, R, roughness * environment.radiance_mip_levels)).rgb;
  vec2 brdf = srgb_to_linear(
      texture(brdf_lut, vec2(max(dot(N, V), 0.0), 1.0 - roughness))).rg;
  vec3 specular = prefiltered_color * (F * brdf.x + brdf.y);

  vec3 ambient = kD * diffuse + specular;

  return vec4(
      (ambient + Lo) * occlusion + emissive,
      albedo.a);
}

#endif
