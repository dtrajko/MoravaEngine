#version 330 core

out vec4 FragColor;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
in vec4 vColor;

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

struct Material
{
	float specularIntensity;
	float shininess;
};

uniform DirectionalLight directionalLight;

uniform sampler2D albedoMap;
uniform vec4 tintColor;

uniform Material material;
uniform vec3 eyePosition; // same as cameraPosition


vec4 CalcLightByDirection(Light light, vec3 direction, float shadowFactor)
{
	vec4 ambientColor = vec4(light.color, 1.0) * light.ambientIntensity;

	float diffuseFactor = max(dot(normalize(vNormal), -normalize(direction)), 0.0);
	vec4 diffuseColor = vec4(light.color, 1.0) * light.diffuseIntensity * diffuseFactor;

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 1.0);

	if (diffuseFactor > 0.0)
	{
		vec3 fragToEye = normalize(eyePosition - vFragPos);
		vec3 reflectedVertex = normalize(reflect(direction, normalize(vNormal)));

		float specularFactor = dot(fragToEye, reflectedVertex);
		if (specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, material.shininess);
			specularColor = vec4(light.color * material.specularIntensity * specularFactor, 1.0f);
		}
	}

	return (ambientColor + (1.0 - shadowFactor) * (diffuseColor + specularColor));
}

vec4 CalcDirectionalLight(vec4 color)
{
	return color * CalcLightByDirection(directionalLight.base, directionalLight.direction, 0.0);
}

void main()
{
    vec4 finalColor = vec4(1.0, 1.0, 1.0, 1.0);
    finalColor = CalcDirectionalLight(finalColor);
    vec4 texColor = texture(albedoMap, vTexCoord);

	FragColor = texColor * finalColor * tintColor * vColor;
}
