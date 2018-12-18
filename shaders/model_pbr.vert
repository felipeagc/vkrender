#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera_ubo;

layout (set = 2, binding = 0) uniform MeshUniform {
  mat4 matrix;
} mesh_ubo;

layout (set = 3, binding = 0) uniform ModelUniform {
  mat4 matrix;
} model_ubo;

layout (location = 0) out vec2 texCoords0;
layout (location = 1) out vec3 worldPos;
layout (location = 2) out vec3 normal0;

void main() {
  texCoords0 = texCoords;

  vec4 locPos;

  // mat4 model = model_ubo.matrix * mesh_ubo.matrix;

  locPos = model_ubo.matrix * vec4(pos, 1.0);
  normal0 = mat3(transpose(inverse(model_ubo.matrix))) * normal;

  worldPos = locPos.xyz / locPos.w;

  // worldPos += vec3(5.0, 2.0, 2.0);
  // gl_Position = camera_ubo.proj * vec4(worldPos, 1.0);
  gl_Position = camera_ubo.proj * camera_ubo.view * vec4(worldPos, 1.0);
}
