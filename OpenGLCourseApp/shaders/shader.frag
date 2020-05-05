#version 330

const int MAX_POINT_LIGHTS = 3;
const int MAX_SPOT_LIGHTS = 3;

const float dirLightShadowIntensity = 0.6;

struct Light
{
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

struct OmniShadowMap
{
	samplerCube shadowMap;
	float farPlane;
};

struct Material
{
	float specularIntensity;
	float shininess;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in vec4 DirectionalLightSpacePos;
in mat3 TBN;

out vec4 color;

uniform int pointLightCount;
uniform int spotLightCount;

uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

uniform sampler2D theTexture;
uniform sampler2D normalMap;
uniform sampler2D directionalShadowMap;
uniform OmniShadowMap omniShadowMaps[MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS];

uniform Material material;
uniform vec3 eyePosition;

uniform float tilingFactor;

vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
	vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
	vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
	vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
	vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

vec3 GetNormal()
{
	vec3 normal = normalize(Normal);
	// return normal;

	normal = texture(normalMap, TexCoords * tilingFactor).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(TBN * normal);
	return normal;
}

float CalcDirectionalShadowFactor(DirectionalLight light)
{
	vec3 projCoords = DirectionalLightSpacePos.xyz / DirectionalLightSpacePos.w;
	projCoords = (projCoords * 0.5) + 0.5;
	
	float closestDepth = texture(directionalShadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(light.direction);

	float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0001);
	
	float shadow = 0.0;
	shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	// PCF method
	if (true)
	{
		vec2 texelSize = 1.0 / textureSize(directionalShadowMap, 0);
		for (int x = -1; x <= 1; ++x)
		{
			for (int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(directionalShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			}
		}
		shadow /= 9.0;
	}

	if (projCoords.z > 1.0)
	{
		shadow = 0.0;
	}

	return shadow;
}

float CalcOmniShadowFactor(PointLight light, int shadowIndex)
{
	float shadow = 0.0;
	float bias = 0.001;
	float samples = 20.0;

	float viewDistance = length(eyePosition - FragPos);
	float diskRadius = (1.0 + (viewDistance / omniShadowMaps[shadowIndex].farPlane)) / 25.0;

	vec3 fragToLight = FragPos - light.position;
	float currentDepth = length(fragToLight);
	float closestDepth = texture(omniShadowMaps[shadowIndex].shadowMap, fragToLight).r;
	closestDepth *= omniShadowMaps[shadowIndex].farPlane;
	shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	// PCF method
	if (true)
	{
		for (int i = 0; i < samples; i++)
		{
			closestDepth = texture(omniShadowMaps[shadowIndex].shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
			closestDepth *= omniShadowMaps[shadowIndex].farPlane;
			if (currentDepth - bias > closestDepth)
			{
				shadow += 1.0;
			}		
		}
		shadow /= samples;
	}
	return shadow;
}

vec4 CalcLightByDirection(Light light, vec3 direction, float shadowFactor)
{
	vec4 ambientColor = vec4(light.color, 1.0) * light.ambientIntensity;

	float diffuseFactor = max(dot(GetNormal(), -normalize(direction)), 0.0);
	vec4 diffuseColor = vec4(light.color, 1.0) * light.diffuseIntensity * diffuseFactor;

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 0.0);

	if (diffuseFactor > 0.0)
	{
		vec3 fragToEye = normalize(eyePosition - FragPos);
		vec3 reflectedVertex = normalize(reflect(direction, GetNormal()));

		float specularFactor = dot(fragToEye, reflectedVertex);
		if (specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, material.shininess);
			specularColor = vec4(light.color * material.specularIntensity * specularFactor, 1.0f);
		}
	}

	return (ambientColor + (1.0 - shadowFactor) * (diffuseColor + specularColor));
}

vec4 CalcDirectionalLight()
{
	float shadowFactor = CalcDirectionalShadowFactor(directionalLight);
	shadowFactor *= dirLightShadowIntensity;
	return CalcLightByDirection(directionalLight.base, directionalLight.direction, shadowFactor);
}

vec4 CalcPointLight(PointLight pointLight, int shadowIndex)
{
	vec3 direction = FragPos - pointLight.position;
	float distance = length(direction);
	direction = normalize(direction);

	float shadowFactor = CalcOmniShadowFactor(pointLight, shadowIndex);

	vec4 color = CalcLightByDirection(pointLight.base, direction, shadowFactor);
	float attenuation =	pointLight.exponent * distance * distance +
						pointLight.linear * distance +
						pointLight.constant;

	return (color / attenuation);
}

vec4 CalcSpotLight(SpotLight spotLight, int shadowIndex)
{
	vec3 rayDirection = normalize(FragPos - spotLight.base.position);
	float spotLightFactor = dot(rayDirection, spotLight.direction);

	if (spotLightFactor > spotLight.edge)
	{
		vec4 color = CalcPointLight(spotLight.base, shadowIndex);
		color *= (1.0 - (1.0 - spotLightFactor) * (1.0 / (1.0 - spotLight.edge)));
		return color;
	}
	else
	{
		return vec4(0, 0, 0, 0);
	}
}

vec4 CalcPointLights()
{
	vec4 totalColor = vec4(0, 0, 0, 0);
	for (int i = 0; i < pointLightCount; i++)
	{
		totalColor += CalcPointLight(pointLights[i], i);
	}
	return totalColor;
}

vec4 CalcSpotLights()
{
	vec4 totalColor = vec4(0, 0, 0, 0);
	for (int i = 0; i < spotLightCount; i++)
	{
		totalColor += CalcSpotLight(spotLights[i], pointLightCount + i);
	}
	return totalColor;
}


void main()
{
	vec4 finalColor = CalcDirectionalLight();
	finalColor += CalcPointLights();
	finalColor += CalcSpotLights();

	vec4 texColor = texture(theTexture, TexCoords * tilingFactor);
	if(texColor.a < 0.1)
        discard;

	color = texColor * finalColor;
}
