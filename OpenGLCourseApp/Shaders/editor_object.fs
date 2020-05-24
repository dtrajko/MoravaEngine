#version 330 core

const int MAX_POINT_LIGHTS = 4;
const int MAX_SPOT_LIGHTS = 4;

const int MAX_LIGHTS = MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPosition;
in vec3 vFragPos;

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

struct PointLight
{
	Light base;
	vec3 position;
	float constant;
	float linear;
	float exponent;
};

struct SpotLight
{
	PointLight base;
	vec3 direction;
	float edge;
};

struct Material
{
	float specularIntensity;
	float shininess;
};

uniform int pointLightCount;
uniform int spotLightCount;

uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

uniform Material material;

uniform samplerCube cubeMap;
uniform sampler2D   albedoMap;
uniform vec4  tintColor;
uniform float tilingFactor;
uniform bool  isSelected;
uniform bool  useCubeMaps;

uniform vec3 eyePosition; // same as cameraPosition

vec4 CalcLightByDirection(Light light, vec3 direction)
{
	vec4 ambientColor = vec4(light.color, 1.0) * light.ambientIntensity;

	float diffuseFactor = max(dot(normalize(vNormal), normalize(direction)), 0.0);
	vec4 diffuseColor = vec4(light.color, 1.0) * light.diffuseIntensity * diffuseFactor;

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 0.0);

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

	return (ambientColor + diffuseColor + specularColor);
}

vec4 CalcDirectionalLight()
{
	return CalcLightByDirection(directionalLight.base, -directionalLight.direction);
}

vec4 CalcPointLight(PointLight pointLight)
{
	if (!pointLight.base.enabled) return vec4(0.0, 0.0, 0.0, 0.0);

	vec3 direction = vFragPos - pointLight.position;
	float distance = length(direction);
	direction = normalize(direction);
	
	vec4 color = CalcLightByDirection(pointLight.base, direction);
	float attenuation =	pointLight.exponent * distance * distance +
						pointLight.linear * distance +
						pointLight.constant;

	return (color / attenuation);
}

vec4 CalcSpotLight(SpotLight spotLight)
{
	if (!spotLight.base.base.enabled) return vec4(0.0, 0.0, 0.0, 0.0);

	vec3 rayDirection = normalize(vFragPos - spotLight.base.position);
	float spotLightFactor = dot(rayDirection, spotLight.direction);

	if (spotLightFactor > spotLight.edge)
	{
		vec4 color = CalcPointLight(spotLight.base);
		color *= (1.0 - (1.0 - spotLightFactor) * (1.0 / (1.0 - spotLight.edge)));
		return color;
	}
	else
	{
		return vec4(0.0, 0.0, 0.0, 0.0);
	}
}

vec4 CalcPointLights()
{
	vec4 totalColor = vec4(0.0, 0.0, 0.0, 0.0);
	for (int i = 0; i < pointLightCount; i++)
	{
		totalColor += CalcPointLight(pointLights[i]);
	}
	return totalColor;
}

vec4 CalcSpotLights()
{
	vec4 totalColor = vec4(0.0, 0.0, 0.0, 0.0);
	for (int i = 0; i < spotLightCount; i++)
	{
		totalColor += CalcSpotLight(spotLights[i]);
	}
	return totalColor;
}

void main()
{
	vec4 CubeMapColor = vec4(1.0, 1.0, 1.0, 1.0);
	if (useCubeMaps) {
		vec3 I = normalize(vPosition - eyePosition);
		vec3 R = reflect(I, normalize(vNormal));
		CubeMapColor = vec4(texture(cubeMap, R).rgb, 1.0);	
	}

	vec4 texColor = texture(albedoMap, vTexCoord * tilingFactor);
	if(texColor.a < 0.1) // enable alpha transparency
		discard;

	vec4 finalColor = CalcDirectionalLight();
	finalColor += CalcPointLights();
	finalColor += CalcSpotLights();

	FragColor = texColor * finalColor * tintColor * CubeMapColor;

	// use a basic color to identify the shader
	if (vFragPos.x > 0.0 && vFragPos.x < 0.1)
	{
		// FragColor = vec4(0.0, 0.0, 1.0, 1.0);
	}
}
