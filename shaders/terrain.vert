#version 450

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "noise.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (push_constant) uniform PushConstant {
  float time;
} pc;

layout (set = 0, binding = 0) uniform CameraUniform {
  Camera camera;
};

layout (set = 2, binding = 0) uniform LocalModelUniform {
  Model local_model;
};

layout (set = 3, binding = 0) uniform ModelUniform {
  Model model;
};

// layout (set = 5, binding = 0) uniform sampler2D heightmap;

layout (location = 0) out vec2 tex_coords0;
layout (location = 1) out vec3 world_pos;
layout (location = 2) out vec3 normal0;
layout (location = 3) out vec3 camera_pos;
layout (location = 4) out float vertex_height;

const float mult = 0.1f;
const float hmult = 0.5f;

vec3 get_pos(vec3 from) {
  return vec3(
      from.x + snoise(from.xz + 5813.0 + pc.time / 10.0) / 50.0,
      snoise(from.xz * mult + vec2(pc.time / 20.0, sin(pc.time / 2.0) / 10.0)) * hmult,
      from.z + snoise(from.xz + 2311.0 + pc.time / 10.0) / 50.0);
}

void main() {
  tex_coords0 = tex_coords;

  mat4 model0 = model.matrix * local_model.matrix;

  vec3 new_pos = get_pos(pos);

  // For vertex color
  vertex_height = (new_pos.y / hmult) + 1.0;

  float left =  get_pos(pos + vec3(-1.0, 0.0, 0.0)).y;
  float right = get_pos(pos + vec3(1.0, 0.0, 0.0)).y;
  float top = get_pos(pos + vec3(0.0, 0.0, -1.0)).y;
  float bottom = get_pos(pos + vec3(0.0, 0.0, 1.0)).y;

  vec3 new_normal = normalize(vec3(
      left - right,
      2.0f,
      top - bottom));

  vec4 loc_pos = model0 * vec4(new_pos, 1.0);
  normal0 = mat3(transpose(inverse(model0))) * new_normal;

  world_pos = loc_pos.xyz / loc_pos.w;

  camera_pos = camera.pos.xyz;

  gl_Position = camera.proj * camera.view * vec4(world_pos, 1.0);
}
