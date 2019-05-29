#version 450

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "normalmap.glsl"

layout (location = 0) in vec2 tex_coords;
layout (location = 1) in vec3 world_pos;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 camera_pos;
layout (location = 4) in float vertex_height;

layout (set = 1, binding = 0) uniform EnvironmentUniform {
  Environment environment;
};
layout (set = 1, binding = 1) uniform samplerCube irradiance_map;
layout (set = 1, binding = 2) uniform samplerCube radiance_map;
layout (set = 1, binding = 3) uniform sampler2D brdf_lut;

layout (set = 3, binding = 0) uniform sampler2D albedo_texture;
layout (set = 3, binding = 1) uniform sampler2D normal_texture;
layout (set = 3, binding = 2) uniform sampler2D metallic_roughness_texture;
layout (set = 3, binding = 3) uniform sampler2D occlusion_texture;
layout (set = 3, binding = 4) uniform sampler2D emissive_texture;
layout (set = 3, binding = 5) uniform MaterialUniform {
  Material material;
};

layout (location = 0) out vec4 out_color;

#include "pbr.glsl"

void main() {
  vec3 N = normal_map(normal, world_pos, material, normal_texture, tex_coords);
  vec3 V = normalize(camera_pos - world_pos);

  vec4 albedo = srgb_to_linear(texture(albedo_texture, tex_coords)) * material.base_color;
  albedo = mix(albedo * vec4(vec3(0.3), 1.0), albedo, vertex_height);

  vec4 metallic_roughness = texture(metallic_roughness_texture, tex_coords);
  float metallic = material.metallic * metallic_roughness.b;
  float roughness = material.roughness * metallic_roughness.g;

  float occlusion = texture(occlusion_texture, tex_coords).r;
  vec3 emissive = srgb_to_linear(texture(emissive_texture, tex_coords)).rgb * material.emissive.rgb;

  // Calculate PBR
  out_color = calculate_pbr(
      world_pos,
      albedo,
      metallic,
      roughness,
      occlusion,
      emissive,
      N,
      V);

  // HDR tonemapping
  out_color.rgb = vec3(1.0) - exp(-out_color.rgb * environment.exposure);

  // Gamma correct
  out_color.rgb = pow(out_color.rgb, vec3(1.0/GAMMA));
}
