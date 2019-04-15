#version 450

#include "common.glsl"

layout (location = 0) in vec3 tex_coords;

layout (set = 1, binding = 0) uniform EnvironmentUniform {
  Environment environment;
};
layout (set = 1, binding = 1) uniform samplerCube env_map;
layout (set = 1, binding = 2) uniform samplerCube irradiance_map;
layout (set = 1, binding = 3) uniform samplerCube radiance_map;
layout (set = 1, binding = 4) uniform sampler2D brdf_lut;

layout (location = 0) out vec4 out_color;

void main() {
  out_color = pow(vec4(texture(env_map, tex_coords).rgb, 1.0), vec4(2.2));
  out_color = vec4(1.0) - exp(-out_color * environment.exposure);
  out_color = pow(out_color, vec4(1.0/2.2));
}
