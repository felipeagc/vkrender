#version 450

layout (location = 0) in vec3 texCoords;

layout (set = 1, binding = 1) uniform samplerCube albedo;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(texture(albedo, texCoords).rgb, 1.0);
}
