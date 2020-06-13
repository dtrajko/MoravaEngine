#version 330

layout (location = 0) in vec3 aPosition;

uniform mat4 model;
uniform mat4 dirLightTransform;


void main()
{
	gl_Position = dirLightTransform * model * vec4(aPosition, 1.0);
}
