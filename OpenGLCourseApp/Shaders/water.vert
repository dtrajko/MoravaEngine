#version 330

layout (location = 0) in vec2 aPosition;

out vec4 clipSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	clipSpace = projection * view * model * vec4(aPosition.x, 0.0, aPosition.y, 1.0);
	gl_Position = clipSpace;
}
