#version 450

layout (location = 0) in vec3 color;

layout (set = 0, binding = 0) uniform UniformBufferObject {
  vec3 tint;
} ubo;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(color * ubo.tint, 1.0);
}
