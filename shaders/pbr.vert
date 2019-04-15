#version 450

#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  Camera camera;
};

layout (set = 2, binding = 0) uniform LocalModelUniform {
  Model local_model;
};

layout (set = 3, binding = 0) uniform ModelUniform {
  Model model;
};

layout (location = 0) out vec2 tex_coords0;
layout (location = 1) out vec3 world_pos;
layout (location = 2) out vec3 normal0;

void main() {
  tex_coords0 = tex_coords;

  mat4 model0 = model.matrix * local_model.matrix;

  vec4 loc_pos = model0 * vec4(pos, 1.0);
  normal0 = mat3(transpose(inverse(model0))) * normal;

  world_pos = loc_pos.xyz / loc_pos.w;

  gl_Position = camera.proj * camera.view * vec4(world_pos, 1.0);
}
