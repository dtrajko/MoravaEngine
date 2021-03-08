#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPosition;
in vec3 vFragPos;

out vec4 FragColor;

uniform samplerCube cubeMap;
uniform sampler2D albedoMap;
uniform sampler2D shadowMap;
uniform sampler2D normalMap;
uniform vec4  tintColor;
uniform float tilingFactor;
uniform bool  isSelected;
uniform bool  useCubeMaps;


void main()
{
	vec4 texColor = texture(albedoMap, vTexCoord * tilingFactor);
	if(texColor.a < 0.1)
        discard;

	FragColor = texColor;
}
