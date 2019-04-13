#version 450

layout (location = 0) out vec4 out_color;

layout (push_constant) uniform PushConstant {
  mat4 mvp;
  vec4 color;
} pc;

void main() {
  out_color = pc.color;
}
