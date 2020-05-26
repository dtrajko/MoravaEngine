#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPosition;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec4  tintColor;


void main()
{
	FragColor = tintColor;
}
