#version 440

out vec4 outColor;

in vec2 vTexCoord1;
in vec2 vTexCoord2;
in float vBlendFactor;
in vec3 vPosition;

uniform sampler2D albedoMap;

void main()
{
	vec4 color1 = texture(albedoMap, vTexCoord1);
	vec4 color2 = texture(albedoMap, vTexCoord2);

	// outColor = mix(color1, color2, vBlendFactor);
	outColor = vec4(vPosition, 1.0);
}
