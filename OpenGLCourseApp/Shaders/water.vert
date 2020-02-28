#version 400 core

const vec2 quadVertices[4] = { vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0) };


void main(void) {
	gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0); 
}
