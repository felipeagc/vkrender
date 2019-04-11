#version 450

layout (push_constant) uniform PickingPushConstant {
  uint index;
} picking;

layout (location = 0) out uint out_color;

void main() {
  out_color = picking.index;
}
