#version 450

#include "common.glsl"

layout (location = 0) in vec3 pos;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  Camera camera;
};

layout (set = 1, binding = 0) uniform LocalModelUniform {
  Model local_model;
};

layout (set = 2, binding = 0) uniform ModelUniform {
  Model model;
};

layout (location = 1) out vec3 world_pos;

void main() {
  mat4 model0 = model.matrix * local_model.matrix;

  vec4 loc_pos = model0 * vec4(pos, 1.0);
  world_pos = loc_pos.xyz / loc_pos.w;

  gl_Position = camera.proj * camera.view * vec4(world_pos, 1.0);
}
