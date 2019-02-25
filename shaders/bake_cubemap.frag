#version 450

layout(location = 0) in vec3 world_pos;

layout(set = 0, binding = 1) uniform sampler2D equirectangular_map;

layout(location = 0) out vec4 out_color;

const vec2 inv_atan = vec2(0.1591, 0.3183);

vec2 sample_spherical_map(vec3 v) {
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= inv_atan;
  uv += 0.5;
  return uv;
}

void main() {
  vec2 uv = sample_spherical_map(normalize(world_pos));
  uv.y = 1.0 - uv.y;
  vec3 color = texture(equirectangular_map, uv).rgb;
  out_color = vec4(color, 1.0);
}
