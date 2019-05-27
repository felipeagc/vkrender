#version 450

#extension GL_GOOGLE_include_directive : require

layout (push_constant) uniform PickingPushConstant {
  uint index;
} picking;

layout (location = 0) out uint out_color;

void main() {
  out_color = picking.index;
}
