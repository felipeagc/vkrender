#version 450

layout (location = 0) in vec3 pos;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (push_constant) uniform PushConstant {
  mat4 mvp;
  vec4 color;
} pc;

void main() {
  gl_Position = pc.mvp * vec4(pos, 1.0);
}
