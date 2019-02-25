#version 450

layout (location = 0) in vec2 tex_coords;

layout (set = 0, binding = 0) uniform sampler2D texture0;

layout (location = 0) out vec4 out_color;

void main() {
  out_color = texture(texture0, tex_coords);
}
