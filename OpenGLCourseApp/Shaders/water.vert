#version 400 core

in vec2 position;

out vec2 textureCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position.x, 0.0, position.y, 1.0);
	textureCoords = vec2(position.x / 2.0 + 0.5, position.y / 2.0 + 0.5);
}
