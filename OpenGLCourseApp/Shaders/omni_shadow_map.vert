#version 330


layout (location = 0) in vec3 aPosition;

uniform mat4 model;

void main()
{
	gl_Position = model * vec4(aPosition, 1.0);
}
