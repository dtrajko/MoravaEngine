#version 330 core

in vec2 TexCoord0;
in vec3 Normal0;
in vec3 WorldPos0;

out vec4 FragColor;

uniform sampler2D gColorMap;


void main()
{
	vec4 texColor = texture(gColorMap, TexCoord0);
	if(texColor.a < 0.1)
        discard;

	FragColor = texColor;
}
