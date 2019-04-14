#version 450

layout (location = 0) out uint out_color;

layout (push_constant) uniform PushConstant {
  mat4 mvp;
  uint index;
} pc;

void main() {
  out_color = pc.index;
}
