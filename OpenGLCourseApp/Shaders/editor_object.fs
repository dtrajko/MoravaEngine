#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;

out vec4 FragColor;

uniform vec4  tintColor;
uniform float tilingFactor;
uniform bool  isSelected;

uniform sampler2D albedoMap;

const float illumination = 1.2;

void main()
{
	vec4 FragColorTex = texture(albedoMap, vTexCoord * tilingFactor);

	vec4 NewColor = FragColorTex * tintColor;
	if (isSelected)
	{
		NewColor.x = min(NewColor.x * illumination, 1.0);
		NewColor.y = min(NewColor.y * illumination, 1.0);
		NewColor.z = min(NewColor.z * illumination, 1.0);
	}
	FragColor = NewColor;
}
