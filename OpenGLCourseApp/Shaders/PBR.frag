#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;

uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform vec3 camPos;

const float PI = 3.14159265359;

float distributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;
	return a2 / max(denom, 0.0000001); // prevent divide by zero
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	float ggx1 = NdotV / (NdotV * (1.0 - k) + k); // Schlick GGX;
	float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float HdotV, vec3 baseReflectivity)
{
	// baseReflectivity in range 0 to 1
	// returns range of baseReflectivity to 1
	// increases as HdotV decreases (more reflectivity when surface viewed at larger angles)
	return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}

void main()
{
	vec3 albedo     = texture(albedoMap,    TexCoords).rgb;
	float metallic  = texture(metallicMap,  TexCoords).r;
	float roughness = texture(roughnessMap, TexCoords).r;
	float ao        = texture(aoMap,        TexCoords).r;

	// vec3 N = normalize(Normal);
	vec3 N = getNormalFromMap();
	vec3 V = normalize(camPos - WorldPos);

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use baseReflectivity
	// of 0.04 and if it's a metal, use the albedo color as baseReflectivity (metallic workflow)
	vec3 baseReflectivity = mix(vec3(0.04), albedo, metallic);

	// reflectance equation
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < 4; ++i)
	{
		// calculate per-light radiance
		vec3 L = normalize(lightPositions[i] - WorldPos);
		vec3 H = normalize(V + L);
		float distance = length(lightPositions[i] - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		// Cook-Torrance BRDF
		float NdotV = max(dot(N, V), 0.0000001); // min of 0.0000001 to prevent divide by zero
		float NdotL = max(dot(N, L), 0.0000001);
		float HdotV = max(dot(H, V), 0.0);
		float NdotH = max(dot(N, H), 0.0);

		float D = distributionGGX(NdotH, roughness); // larger the more micro-facets alight to H (normal distribution function)
		float G = geometrySmith(NdotV, NdotL, roughness); // smaller the more micro-facets shadowed by other micro-facets
		vec3 F = fresnelSchlick(HdotV, baseReflectivity); // proportion of specular reflectance

		vec3 specular = D * G * F;
		specular /= 4.0 * NdotV * NdotL;

		// for energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - F; // F equals kS

		// multiply kD by the inverse metalness such that only non-metals
		// have diffuse lighting, or a linear blend if partly metal
		// (pure metals have no diffuse light).
		kD *= 1.0 - metallic;

		// note that 1) angle of light to surface affects specular, not just diffuse
		//           2) we mix albedo with diffuse, but not specular
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	// ambient lighting (note that the next IBL tutorial will replace
	// this ambient lighting with environment lighting)
	vec3 ambient = vec3(0.03) * albedo;

	vec3 color = ambient + Lo;

	// HDR tonemapping
	color = color / (color + vec3(1.0));

	// gamma correct
	color = pow(color, vec3(1.0 / 2.2));

	FragColor = vec4(color, 1.0);
}
