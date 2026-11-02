#version 330

layout (location = 0) in vec3 pos;

out vec3 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	TexCoords = pos;
	gl_Position = projection * view * model * vec4(pos, 1.0);
}
