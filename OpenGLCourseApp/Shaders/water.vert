#version 400 core

layout (location = 0) in vec3 pos;

const vec2 quadVertices[4] = { vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0) };

out vec2 TexCoord;

void main(void) {
	gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0);
	TexCoord = vec2((quadVertices[gl_VertexID].x + 1.0) / 2.0, (quadVertices[gl_VertexID].y + 1.0) / 2.0);
}
