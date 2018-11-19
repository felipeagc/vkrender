#version 450

#define MAX_LIGHTS 20

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

struct Light {
  vec4 pos;
  vec4 color;
};

layout(set = 3, binding = 0) uniform LightingUniform {
  Light lights[MAX_LIGHTS];
  uint lightCount;
} lighting;

layout (location = 0) out vec4 outColor;

vec3 calcLight(Light light, vec3 normal, vec3 viewDir) {
  // ambient
  float ambientStrength = 0.3;
  // vec3 ambient = ambientStrength * light.color.rgb;

  // diffuse
  vec3 lightDir = normalize(light.pos.xyz - fragPos);
  float diff = max(dot(normal, lightDir), 0.0);
  // vec3 diffuse = diff * light.color.rgb;

  // specular
  float specularStrength = 0.5;
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
  // vec3 specular = specularStrength * spec * light.color.rgb;

  vec3 ambient = ambientStrength * light.color.rgb * vec3(texture(albedo, texCoords));
  vec3 diffuse = light.color.rgb * diff * vec3(texture(albedo, texCoords));
  vec3 specular = light.color.rgb * spec * specularStrength * vec3(texture(albedo, texCoords));

  return (ambient + diffuse + specular) * material.color.rgb;
}

void main() {
  vec3 viewPos = vec3(camera.view[3][0], camera.view[3][1], camera.view[3][2]);

  vec3 viewDir = normalize(viewPos - fragPos);
  vec3 norm = normalize(normal);

  vec3 result = vec3(0.0, 0.0, 0.0);

  for (int i = 0; i < lighting.lightCount; i++) {
    result += calcLight(lighting.lights[i], norm, viewDir);
  }

  outColor = vec4(result, 1.0);
}
