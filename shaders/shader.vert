#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (location = 0) out vec3 color0;
layout (location = 1) out vec2 texCoords0;

void main() {
  gl_Position = vec4(pos, 1.0);
  texCoords0 = texCoords;
  color0 = color;
}
