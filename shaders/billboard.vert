#version 450

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

const vec2 pos[6] = vec2[](
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

const vec2 tex_coords[6] = vec2[](
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
  Camera camera;
};

layout (push_constant) uniform BillboardUniform {
  mat4 model;
  vec4 color;
} billboard;

layout (location = 0) out vec2 tex_coords0;

void main() {
  mat4 inv_view = inverse(camera.view);
  vec3 camera_right_world_space = vec3(inv_view[0][0], inv_view[0][1], inv_view[0][2]);
  vec3 camera_up_world_space = -vec3(inv_view[1][0], inv_view[1][1], inv_view[1][2]);

  vec3 world_pos =
    camera_right_world_space * pos[gl_VertexIndex].x +
    camera_up_world_space * pos[gl_VertexIndex].y;

  gl_Position = camera.proj * camera.view * billboard.model * vec4(world_pos, 1.0);
  tex_coords0 = tex_coords[gl_VertexIndex];
  tex_coords0.y = 1.0 - tex_coords0.y;
}
