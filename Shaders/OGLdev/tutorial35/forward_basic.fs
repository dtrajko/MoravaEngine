#version 330 core

in vec2 TexCoord0;
in vec3 Normal0;
in vec3 WorldPos0;

layout (location = 0) out vec4 DiffuseOut;

uniform sampler2D gColorMap;

void main()
{	
	DiffuseOut = texture(gColorMap, TexCoord0);
}
