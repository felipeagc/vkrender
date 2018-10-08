#version 450

layout (location = 0) in vec3 color;
layout (location = 1) in vec2 texCoords;

layout (set = 0, binding = 0) uniform UniformBufferObject {
  vec3 tint;
} ubo;

layout (set = 0, binding = 1) uniform sampler2D texSampler;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = texture(texSampler, texCoords) * vec4(color * ubo.tint, 1.0);
}
