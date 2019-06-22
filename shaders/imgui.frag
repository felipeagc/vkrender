#version 450 core

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform sampler2D atlas;

layout (location = 0) in struct {
  vec4 color;
  vec2 uv;
} in0;

void main() {
  out_color = in0.color * texture(atlas, in0.uv.st);
}
