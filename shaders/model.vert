#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera_ubo;

layout (set = 2, binding = 0) uniform ModelUniform {
  mat4 model;
} model_ubo;

layout (location = 0) out vec2 texCoords0;

void main() {
  gl_Position = camera_ubo.proj * camera_ubo.view * model_ubo.model * vec4(pos, 1.0);
  texCoords0 = texCoords;
}
