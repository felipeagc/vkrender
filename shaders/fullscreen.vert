#version 450 

out gl_PerVertex {
  vec4 gl_Position;
};

layout (location = 0) out vec2 texCoords;

void main() {
	texCoords = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(texCoords * 2.0f + -1.0f, 0.0f, 1.0f);
}
