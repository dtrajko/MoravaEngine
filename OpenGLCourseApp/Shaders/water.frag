#version 400 core

in vec3 vPosition;
in vec2 TexCoord;

out vec4 out_Color;


void main(void)
{
	out_Color = vec4(vPosition, 1.0);
}
