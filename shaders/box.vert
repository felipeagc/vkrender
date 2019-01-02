#version 450

layout (location = 0) in vec3 pos;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera_ubo;

layout (set = 1, binding = 0) uniform ModelUniform {
  mat4 matrix;
} model_ubo;

layout (push_constant) uniform PushConstant {
  vec3 scale;
  layout(offset = 16) vec3 offset;
} pc;

void main() {
  vec4 locPos = model_ubo.matrix * vec4(pos * pc.scale + pc.offset, 1.0);
  vec3 worldPos = locPos.xyz / locPos.w;

  gl_Position = camera_ubo.proj * camera_ubo.view * vec4(worldPos, 1.0);
}
