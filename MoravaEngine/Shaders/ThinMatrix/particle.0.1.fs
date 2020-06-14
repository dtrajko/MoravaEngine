#version 440

out vec4 outColor;

in vec2 vTexCoord;

uniform sampler2D albedoMap;

void main()
{
	outColor = texture(albedoMap, vTexCoord);
}
