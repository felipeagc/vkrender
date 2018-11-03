#version 450

layout (location = 0) in vec2 texCoords;

layout (set = 1, binding = 0) uniform sampler2D albedo;

layout (set = 1, binding = 1) uniform MaterialUniform {
    vec4 color;
} material;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = material.color * texture(albedo, texCoords);
}
