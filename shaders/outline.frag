#version 450

#include "common.glsl"

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant {
  vec4 color;
} pc;

void main() {
  out_color = pc.color;
}
