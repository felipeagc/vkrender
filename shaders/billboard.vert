#version 450

vec2 pos[6] = vec2[](
  // Bottom left
  vec2(-0.5, 0.5),
  // Bottom right
  vec2(0.5, 0.5),
  // Top right
  vec2(0.5, -0.5),

  // Top right
  vec2(0.5, -0.5),
  // Top left
  vec2(-0.5, -0.5),
  // Bottom left
  vec2(-0.5, 0.5)
);

vec2 texCoords[6] = vec2[](
  // Bottom left
  vec2(0.0, 0.0),
  // Bottom right
  vec2(1.0, 0.0),
  // Top right
  vec2(1.0, 1.0),

  // Top right
  vec2(1.0, 1.0),
  // Top left
  vec2(0.0, 1.0),
  // Bottom left
  vec2(0.0, 0.0)
);

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
  vec3 pos;
} camera_ubo;

layout (set = 2, binding = 0) uniform ModelUniform {
  mat4 model;
} model_ubo;

layout (location = 0) out vec2 texCoords0;

void main() {
  mat4 invView = inverse(camera_ubo.view);
  vec3 cameraRightWorldSpace = vec3(invView[0][0], invView[0][1], invView[0][2]);
  vec3 cameraUpWorldSpace = -vec3(invView[1][0], invView[1][1], invView[1][2]);

  vec3 worldPos =
    cameraRightWorldSpace * pos[gl_VertexIndex].x +
    cameraUpWorldSpace * pos[gl_VertexIndex].y;

  gl_Position = camera_ubo.proj * camera_ubo.view * model_ubo.model * vec4(worldPos, 1.0);
  texCoords0 = texCoords[gl_VertexIndex];
  texCoords0.y = 1.0 - texCoords0.y;
}
