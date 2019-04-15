#version 450

#include "common.glsl"

layout (location = 0) in vec3 pos;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  Camera camera;
};

layout (set = 1, binding = 0) uniform ModelUniform {
  Model model;
};

layout (push_constant) uniform PushConstant {
  vec3 scale;
  layout(offset = 16) vec3 offset;
} pc;

void main() {
  vec4 world_pos = model.matrix * vec4(pos * pc.scale + pc.offset, 1.0);

  gl_Position = camera.proj * camera.view * world_pos;
}
