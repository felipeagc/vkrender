#version 450

layout (location = 0) in vec3 pos;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera_ubo;

layout (set = 1, binding = 0) uniform LocalModelUniform {
  mat4 matrix;
} local_model_ubo;

layout (set = 2, binding = 0) uniform ModelUniform {
  mat4 matrix;
} model_ubo;

layout (location = 1) out vec3 world_pos;

void main() {
  mat4 model = model_ubo.matrix * local_model_ubo.matrix;

  vec4 loc_pos = model * vec4(pos, 1.0);
  world_pos = loc_pos.xyz / loc_pos.w;

  gl_Position = camera_ubo.proj * camera_ubo.view * vec4(world_pos, 1.0);
}
