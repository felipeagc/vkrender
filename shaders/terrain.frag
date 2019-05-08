#version 450

#include "common.glsl"

layout (location = 0) in vec2 tex_coords;
layout (location = 1) in vec3 world_pos;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 camera_pos;

layout (set = 1, binding = 0) uniform EnvironmentUniform {
  Environment environment;
};
layout (set = 1, binding = 1) uniform samplerCube env_map;
layout (set = 1, binding = 2) uniform samplerCube irradiance_map;
layout (set = 1, binding = 3) uniform samplerCube radiance_map;
layout (set = 1, binding = 4) uniform sampler2D brdf_lut;

layout (set = 4, binding = 0) uniform sampler2D albedo_texture;
layout (set = 4, binding = 1) uniform sampler2D normal_texture;
layout (set = 4, binding = 2) uniform sampler2D metallic_roughness_texture;
layout (set = 4, binding = 3) uniform sampler2D occlusion_texture;
layout (set = 4, binding = 4) uniform sampler2D emissive_texture;
layout (set = 4, binding = 5) uniform MaterialUniform {
  Material material;
};

layout (location = 0) out vec4 out_color;

void main() {
  // diffuse 
  float diff = max(dot(normalize(normal), normalize(-environment.sun_direction)), 0.0);
  vec3 diffuse = vec3(diff) * material.base_color.xyz;

  for (int i = 0; i < environment.light_count; i++) {
    vec3 light_pos = environment.lights[i].pos.xyz;
    float diff = max(dot(normalize(normal), normalize(light_pos - world_pos)), 0.0);
    float distance = length(light_pos - world_pos);
    float attenuation = 1.0 / (distance * distance);
    diffuse += diff * environment.lights[i].color.xyz * attenuation;
    diffuse += diff * material.base_color.xyz * attenuation;
  }
  
  // specular
  /* vec3 viewDir = normalize(viewPos - FragPos); */
  /* vec3 reflect_dir = reflect(-environment.sun_direction, norm); */  
  /* float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess); */
  /* vec3 specular = light.specular * (spec * material.specular); */  
      
  vec3 result = diffuse;

  out_color = vec4(result, 1.0);
}
