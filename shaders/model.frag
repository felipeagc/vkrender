#version 450

layout (location = 0) in vec2 texCoords;

layout (set = 1, binding = 0) uniform sampler2D albedo;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = texture(albedo, vec2(texCoords.x, 1.0 - texCoords.y));
}
