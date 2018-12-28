#version 450

layout (location = 0) in vec2 texCoords;

layout (set = 0, binding = 0) uniform sampler2D texture0;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = texture(texture0, texCoords);
}
