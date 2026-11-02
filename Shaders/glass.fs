#version 330

in vec3  vReflection;
in vec3  vRefraction;
in float vFresnel;
in vec2  vTexCoord;
in vec3  vWorldPos;

uniform samplerCube uCubemap;

out vec4 FragColor;

void main(void)
{
	vec4 refractionColor = texture(uCubemap, normalize(vRefraction));
	vec4 reflectionColor = texture(uCubemap, normalize(vReflection));

	FragColor = refractionColor; // mix(refractionColor, reflectionColor, vFresnel);
}
