#version 440

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 modelView;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = projection * modelView * vec4(aPosition, 1.0);
}
