#version 330 core

const int MAX_POINT_LIGHTS = 4;
const int MAX_SPOT_LIGHTS = 4;

const int MAX_LIGHTS = MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS;

const float shadowIntensity = 0.8;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vPosition;
in vec3 vFragPos;
in vec4 vDirLightSpacePos;

out vec4 FragColor;

// lights
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

struct PointSpotLight
{
    Light base;
    vec3 position;
    float exponent;
    float linear;
    float constant;
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

// IBL
uniform samplerCube irradianceMap; // slot 0
uniform samplerCube prefilterMap;  // slot 1
uniform sampler2D brdfLUT;         // slot 2

// material parameters
uniform sampler2D albedoMap;       // slot 3
uniform sampler2D normalMap;       // slot 4
uniform sampler2D metallicMap;     // slot 5
uniform sampler2D roughnessMap;    // slot 6
uniform sampler2D aoMap;           // slot 7

// Directional Light Shadows
uniform sampler2D shadowMap;       // slot 8

uniform int pointSpotLightCount; // both point and spot lights

uniform DirectionalLight directionalLight;
uniform PointSpotLight pointSpotLights[MAX_LIGHTS];

uniform Material material;

uniform vec3 eyePosition; // same as cameraPosition
uniform float tilingFactor;

uniform OmniShadowMap omniShadowMaps[MAX_LIGHTS];

uniform float waterLevel;
uniform vec4 waterColor;

vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
	vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
	vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
	vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
	vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, vTexCoord * tilingFactor).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(vFragPos);
    vec3 Q2  = dFdy(vFragPos);
    vec2 st1 = dFdx(vTexCoord * tilingFactor);
    vec2 st2 = dFdy(vTexCoord * tilingFactor);

    vec3 N   = normalize(vNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float CalcOmniShadowFactor(PointSpotLight pointSpotLight, int shadowIndex)
{
	float shadow = 0.0;
	float bias = 0.001;
	float samples = 20.0;

	float viewDistance = length(eyePosition - vFragPos);
	float diskRadius = (1.0 + (viewDistance / omniShadowMaps[shadowIndex].farPlane)) / 25.0;

	vec3 fragToLight = vFragPos - pointSpotLight.position;
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

float CalcDirectionalShadowFactor(DirectionalLight light)
{
	vec3 projCoords = vDirLightSpacePos.xyz / vDirLightSpacePos.w;
	projCoords = (projCoords * 0.5) + 0.5;
	
	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	vec3 normal = normalize(vNormal);
	vec3 lightDir = normalize(light.direction);

	float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0001);
	
	float shadow = 0.0;
	shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	// PCF method
	if (true)
	{
		vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
		for (int x = -1; x <= 1; ++x)
		{
			for (int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
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

vec4 CalcLightByDirection(Light light, vec3 direction, float shadowFactor)
{
	vec4 ambientColor = vec4(light.color, 1.0) * light.ambientIntensity;

	float diffuseFactor = max(dot(normalize(vNormal), normalize(direction)), 0.0);
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
	float shadowFactor = CalcDirectionalShadowFactor(directionalLight);
	shadowFactor *= shadowIntensity;
	return color * CalcLightByDirection(directionalLight.base, -directionalLight.direction, shadowFactor);
}

vec4 CalcPointSpotLight(PointSpotLight pointSpotLight, int shadowIndex)
{
	if (!pointSpotLight.base.enabled) return vec4(0.0, 0.0, 0.0, 1.0);

	vec3 direction = vFragPos - pointSpotLight.position;
	float distance = length(direction);
	direction = normalize(direction);

	float shadowFactor = CalcOmniShadowFactor(pointSpotLight, shadowIndex);

	vec4 color = CalcLightByDirection(pointSpotLight.base, direction, shadowFactor);
	float attenuation =	pointSpotLight.exponent * distance * distance +
						pointSpotLight.linear * distance +
						pointSpotLight.constant;

	return (color / attenuation);
}

vec4 CalcPointSpotLights()
{
	vec4 totalColor = vec4(0.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < pointSpotLightCount; i++)
	{
		totalColor += CalcPointSpotLight(pointSpotLights[i], i);
	}
	return totalColor;
}

void main()
{
    // material properties
    vec3 albedo     = pow(texture(albedoMap, vTexCoord * tilingFactor).rgb, vec3(2.2));
    float metallic  = texture(metallicMap,   vTexCoord * tilingFactor).r;
    float roughness = texture(roughnessMap,  vTexCoord * tilingFactor).r;
    float ao = texture(aoMap, vTexCoord * tilingFactor).r;

    // input lighting data
    vec3 N = getNormalFromMap();
    vec3 V = normalize(eyePosition - vFragPos);
    vec3 R = reflect(-V, N); 

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < pointSpotLightCount; ++i)
    {
        // skip light calculation if light is not enabled
        if (!pointSpotLights[i].base.enabled) continue;

        // calculate per-light radiance
        vec3 L = normalize(pointSpotLights[i].position - vFragPos);
        vec3 H = normalize(V + L);
        float distance = length(pointSpotLights[i].position - vFragPos);

        // float attenuation = 1.0 / (distance * distance);
        float attenuation =	pointSpotLights[i].exponent * distance * distance +
					pointSpotLights[i].linear * distance +
					pointSpotLights[i].constant;

        vec3 radiance = pointSpotLights[i].base.color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 nominator    = NDF * G * F;
        float denominator = pointSpotLightCount * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
        vec3 specular = nominator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    //  if (directionalLight.base.enabled)
    //  {
    //      color = CalcDirectionalLight(vec4(color, 1.0)).rgb;
    //  }
	//  color += CalcPointSpotLights().xyz;

    FragColor = vec4(color, 1.0);

	// Add a blue tint under the water level
    if (eyePosition.y < waterLevel)
	{
		FragColor = mix(FragColor, waterColor, 0.5);
	}

    // use a basic color to identify the shader
	if (vFragPos.x > 0.0 && vFragPos.x < 0.1)
	{
		// FragColor = vec4(1.0, 0.0, 1.0, 1.0);
	}
}
