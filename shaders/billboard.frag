#version 450

layout (location = 0) in vec2 tex_coords;

layout (push_constant) uniform BillboardUniform {
  mat4 model;
  vec4 color;
} billboard;

layout (set = 1, binding = 0) uniform sampler2D albedo;

layout (location = 0) out vec4 out_color;

void main() {
  out_color = billboard.color * texture(albedo, tex_coords);
}
