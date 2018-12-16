#version 450

layout (location = 0) in vec3 texCoords;

layout (set = 1, binding = 0) uniform EnvironmentUniform {
  float exposure;
} environment;

layout (set = 1, binding = 1) uniform samplerCube envMap;

layout (location = 0) out vec4 outColor;

void main() {
  outColor = pow(vec4(texture(envMap, texCoords).rgb, 1.0), vec4(2.2));
  outColor = vec4(1.0) - exp(-outColor * environment.exposure);
  outColor = pow(outColor, vec4(1.0/2.2));
}
