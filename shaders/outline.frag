#version 450

#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout (location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstant {
  vec4 color;
} pc;

void main() {
  out_color = pc.color;
}
