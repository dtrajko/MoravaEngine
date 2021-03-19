#version 330 core

in vec3 vNormal;

out vec4 FragColor;

struct Light
{
	bool enabled;
	vec3 color;
	float ambientIntensity;
	float diffuseIntensity;
};

struct DirectionalLight
{
	Light base;
	vec3 direction;
};

uniform DirectionalLight directionalLight;

uniform vec4 tintColor;


vec4 CalcLightByDirection(Light light, vec3 direction, float shadowFactor)
{
	vec4 ambientColor = vec4(light.color, 1.0) * light.ambientIntensity;

	float diffuseFactor = max(dot(normalize(vNormal), normalize(direction)), 0.0);
	vec4 diffuseColor = vec4(light.color, 1.0) * light.diffuseIntensity * diffuseFactor;

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 0.0);

	return (ambientColor + (1.0 - shadowFactor) * (diffuseColor + specularColor));
}

vec4 CalcDirectionalLight(vec4 color)
{
	float shadowFactor = 0.0;
	return color * CalcLightByDirection(directionalLight.base, -directionalLight.direction, shadowFactor);
}

void main()
{
	vec4 LightColor = CalcDirectionalLight(tintColor);
	FragColor = vec4(LightColor.xyz, 1.0);
}
