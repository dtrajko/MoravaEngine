#version 330 core

in vec4 vColor;
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

struct DirectionalLight
{
	vec3 color;
	float ambientIntensity;
	vec3 direction;
	float diffuseIntensity;
};

struct Material
{
	float specularIntensity;
	float shininess;
};

uniform sampler2D albedoMap;
uniform vec4  tintColor;
uniform float tilingFactor;
uniform bool  isSelected;

uniform DirectionalLight directionalLight;
uniform Material material;
uniform vec3 eyePosition;

const float illumination = 1.2;

void main()
{
	vec4 ambientColor = vec4(directionalLight.color, 1.0) * directionalLight.ambientIntensity;

	float diffuseFactor = max(dot(normalize(vNormal), normalize(directionalLight.direction)), 0.0);

	vec4 diffuseColor = vec4(directionalLight.color, 1.0) * directionalLight.diffuseIntensity * diffuseFactor;

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 0.0);

	if (diffuseFactor > 0.0)
	{
		vec3 fragToEye = normalize(eyePosition - vFragPos);
		vec3 reflectedVertex = normalize(reflect(directionalLight.direction, normalize(vNormal)));

		float specularFactor = dot(fragToEye, reflectedVertex);
		if (specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, material.shininess);
			specularColor = vec4(directionalLight.color * material.specularIntensity * specularFactor, 1.0f);
		}
	}

	vec4 FragColorTex = texture(albedoMap, vTexCoord * tilingFactor);

	vec4 NewColor = FragColorTex * tintColor;
	if (isSelected)
	{
		NewColor.x = min(NewColor.x * illumination, 1.0);
		NewColor.y = min(NewColor.y * illumination, 1.0);
		NewColor.z = min(NewColor.z * illumination, 1.0);
	}

	FragColor = NewColor * (ambientColor + diffuseColor + specularColor);
}
