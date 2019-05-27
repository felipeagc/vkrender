#version 450

#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec2 tex_coords;

layout (push_constant) uniform BillboardUniform {
  mat4 model;
  uint index;
} billboard;

layout (set = 1, binding = 0) uniform sampler2D albedo;

layout (location = 0) out uint out_color;

void main() {
  if (texture(albedo, tex_coords).a > 0.9f) {
    out_color = billboard.index;
  } else {
    out_color = -1;
  }
}
