#version 450

layout (location = 0) in vec2 texCoords;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 normal;

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera;

layout (set = 1, binding = 0) uniform sampler2D albedo;

layout (set = 1, binding = 1) uniform MaterialUniform {
  vec4 color;
} material;

layout(set = 3, binding = 0) uniform LightingUniform {
  vec4 color;
  vec4 pos;
} lighting;

layout (location = 0) out vec4 outColor;

void main() {
  vec3 viewPos = vec3(camera.view[3][0], camera.view[3][1], camera.view[3][2]);

  // ambient
  float ambientStrength = 0.3;
  vec3 ambient = ambientStrength * lighting.color.rgb;

  // diffuse
  vec3 norm = normalize(normal);
  vec3 lightDir = normalize(lighting.pos.xyz - fragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lighting.color.rgb;

  // specular
  float specularStrength = 0.5;
  vec3 viewDir = normalize(viewPos - fragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  vec3 specular = specularStrength * spec * lighting.color.rgb;

  vec3 result = (ambient + diffuse + specular) * material.color.rgb;
  outColor = vec4(result, 1.0) * texture(albedo, texCoords);
}
