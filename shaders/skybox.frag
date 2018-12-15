#version 450

layout (location = 0) in vec3 texCoords;

layout (set = 1, binding = 0) uniform samplerCube envMap;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = vec4(texture(envMap, texCoords).rgb, 1.0);
}
