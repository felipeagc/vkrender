#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (set = 0, binding = 0) uniform CameraUniform {
  mat4 view;
  mat4 proj;
} camera_ubo;

layout (set = 2, binding = 0) uniform LocalModelUniform {
  mat4 matrix;
} local_model_ubo;

layout (set = 3, binding = 0) uniform ModelUniform {
  mat4 matrix;
} model_ubo;

layout (set = 5, binding = 0) uniform sampler2D heightmap;

layout (location = 0) out vec2 tex_coords0;
layout (location = 1) out vec3 world_pos;
layout (location = 2) out vec3 normal0;

void main() {
  tex_coords0 = tex_coords;

  mat4 model = model_ubo.matrix * local_model_ubo.matrix;

  vec4 loc_pos = model * vec4(pos, 1.0);
  normal0 = mat3(transpose(inverse(model))) * normal;

  world_pos = loc_pos.xyz / loc_pos.w;

  world_pos.y += texture(heightmap, tex_coords).r;

  gl_Position = camera_ubo.proj * camera_ubo.view * vec4(world_pos, 1.0);
}
