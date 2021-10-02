// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated.
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michal Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)
// #type fragment
#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int LightCount = 1;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct DirectionalLight
{
	vec3 Direction;
	vec3 Radiance;
	float Multiplier;
};

in VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 DirLightSpacePos;
	vec3 FragPos;
	mat3 TBN;
	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
} vs_Input;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 o_BloomColor;

uniform DirectionalLight u_DirectionalLights;
uniform vec3 u_CameraPosition;

// PBR texture inputs
uniform sampler2D u_AlbedoTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_MetalnessTexture;
uniform sampler2D u_RoughnessTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_AOTexture;

// Environment maps
uniform samplerCube u_EnvRadianceTex;
uniform samplerCube u_EnvIrradianceTex;

// BRDF LUT
uniform sampler2D u_BRDFLUTTexture;

// PCSS
uniform sampler2D u_ShadowMapTexture[4];
uniform mat4 u_LightView;
uniform bool u_ShowCascades;
uniform bool u_SoftShadows;
uniform float u_LightSize;
uniform float u_MaxShadowDistance;
uniform float u_ShadowFade;
uniform bool u_CascadeFading;
uniform float u_CascadeTransitionFade;

uniform vec4 u_CascadeSplits;

uniform float u_IBLContribution;

uniform float u_BloomThreshold;

////////////////////////////////////////

// uniform vec3 u_AlbedoColor; // TODO: move to Material struct
// uniform float u_Metalness;  // TODO: move to Material struct
// uniform float u_Roughness;  // TODO: move to Material struct
// uniform float u_Emissive;   // TODO: move to Material struct
// uniform float u_AO;         // TODO: move to Material struct

// uniform float u_EnvMapRotation; // TODO: move to Material struct

// Toggles
// uniform float u_RadiancePrefilter;  // TODO: move to Material struct
// uniform float u_AlbedoTexToggle;    // TODO: move to Material struct
// uniform float u_NormalTexToggle;    // TODO: move to Material struct
// uniform float u_MetalnessTexToggle; // TODO: move to Material struct
// uniform float u_RoughnessTexToggle; // TODO: move to Material struct
// uniform float u_EmissiveTexToggle;  // TODO: move to Material struct
// uniform float u_AOTexToggle;        // TODO: move to Material struct

uniform float u_Exposure;

uniform float u_TilingFactor;

struct Material
{
	vec3 AlbedoColor;
	float Metalness;
	float Roughness;
	float Emissive;
	float AO;

	float EnvMapRotation;

	// Toggles
	float RadiancePrefilter;
	float AlbedoTexToggle;
	float NormalTexToggle;
	float MetalnessTexToggle;
	float RoughnessTexToggle;
	float EmissiveTexToggle;
	float AOTexToggle;
};

uniform Material u_MaterialUniforms;

struct PBRParameters
{
	vec3 Albedo;
	vec3 Normal;
	float Metalness;
	float Roughness;
	vec3 Emissive;
	float AO;

	vec3 View;
	float NdotV;

	float ShadowFactor;
	vec4 PointLights;
	vec4 SpotLights;
};

PBRParameters m_Params;

struct WireframeMode
{
	vec4 LineColor;
	bool Enabled;
};

uniform WireframeMode u_WireframeMode;

// ---- BEGIN Phong lighting model ----

const int MAX_POINT_LIGHTS = 1;
const int MAX_SPOT_LIGHTS = 1;
const int MAX_LIGHTS = MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS;

const float material_Shininess = 256.0f;
const float material_SpecularIntensity = 1.0;

const float shadowIntensity = 1.6f;

struct LightBase
{
	bool enabled;
	vec3 color;
	float ambientIntensity;
	float diffuseIntensity;
};

struct PointLight
{
	LightBase base;
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

uniform int pointLightCount;
uniform int spotLightCount;

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];

// Shadow Map Directional Light
uniform sampler2D u_ShadowMap;

struct OmniShadowMap
{
	samplerCube shadowMap;
	float farPlane;
};

uniform OmniShadowMap u_OmniShadowMaps[MAX_LIGHTS];

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
	vec3 normal = normalize(vs_Input.Normal);
	// return normal;

	normal = texture(u_NormalTexture, vs_Input.TexCoord * u_TilingFactor).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(vs_Input.TBN * normal);
	return normal;
}

float CalcOmniShadowFactor(PointLight light, int shadowIndex)
{
	float shadow = 0.0;
	float bias = 0.001;
	float samples = 20.0;

	float viewDistance = length(u_CameraPosition - vs_Input.FragPos);
	float diskRadius = (1.0 + (viewDistance / u_OmniShadowMaps[shadowIndex].farPlane)) / 25.0;

	vec3 fragToLight = vs_Input.FragPos - light.position;
	float currentDepth = length(fragToLight);
	float closestDepth = texture(u_OmniShadowMaps[shadowIndex].shadowMap, fragToLight).r;
	closestDepth *= u_OmniShadowMaps[shadowIndex].farPlane;
	shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	// PCF method
	if (true)
	{
		for (int i = 0; i < samples; i++)
		{
			closestDepth = texture(u_OmniShadowMaps[shadowIndex].shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
			closestDepth *= u_OmniShadowMaps[shadowIndex].farPlane;
			if (currentDepth - bias > closestDepth)
			{
				shadow += 1.0;
			}		
		}
		shadow /= samples;
	}

	shadow *= shadowIntensity;

	return shadow;
}

float CalcDirectionalShadowFactor()
{
	vec3 projCoords = vs_Input.DirLightSpacePos.xyz / vs_Input.DirLightSpacePos.w;
	projCoords = (projCoords * 0.5) + 0.5;

	float closestDepth = texture(u_ShadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	vec3 normal = normalize(m_Params.Normal);
	vec3 lightDir = normalize(u_DirectionalLights.Direction);

	float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0001);
	
	float shadow = 0.0;
	shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	// PCF method
	if (true)
	{
		vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
		for (int x = -1; x <= 1; ++x)
		{
			for (int y = -1; y <= 1; ++y)
			{
				float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
				shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			}
		}
		shadow /= 9.0;
	}

	if (projCoords.z > 1.0)
	{
		shadow = 0.0;
	}

	shadow *= shadowIntensity;

	return shadow;
}

vec4 CalcLightByDirection(LightBase light, vec3 direction, float shadowFactor)
{
	vec4 ambientColor = vec4(light.color, 1.0) * light.ambientIntensity;

	float diffuseFactor = max(dot(GetNormal(), -normalize(direction)), 0.0);
	vec4 diffuseColor = vec4(light.color, 1.0) * light.diffuseIntensity * diffuseFactor;

	vec4 specularColor = vec4(0.0, 0.0, 0.0, 1.0);

	if (diffuseFactor > 0.0)
	{
		vec3 fragToEye = normalize(u_CameraPosition - vs_Input.FragPos);
		vec3 reflectedVertex = normalize(reflect(direction, GetNormal()));

		float specularFactor = dot(fragToEye, reflectedVertex);
		if (specularFactor > 0.0)
		{
			specularFactor = pow(specularFactor, material_Shininess);
			specularColor = vec4(light.color * material_SpecularIntensity * specularFactor, 1.0f);
		}
	}

	return (ambientColor + (1.0 - shadowFactor) * (diffuseColor + specularColor));
}

vec4 CalcPointLight(PointLight pointLight, int shadowIndex)
{
	if (!pointLight.base.enabled) return vec4(0.0, 0.0, 0.0, 1.0);

	vec3 direction = vs_Input.FragPos - pointLight.position;
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
	if (!spotLight.base.base.enabled) return vec4(0.0, 0.0, 0.0, 1.0);

	vec3 rayDirection = normalize(vs_Input.FragPos - spotLight.base.position);
	float spotLightFactor = dot(rayDirection, spotLight.direction);

	if (spotLightFactor > spotLight.edge)
	{
		vec4 color = CalcPointLight(spotLight.base, shadowIndex);
		color *= (1.0 - (1.0 - spotLightFactor) * (1.0 / (1.0 - spotLight.edge)));
		return color;
	}
	else
	{
		return vec4(0.0, 0.0, 0.0, 1.0);
	}
}

vec4 CalcPointLights()
{
	vec4 totalColor = vec4(0.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < pointLightCount; i++)
	{
		totalColor += CalcPointLight(pointLights[i], i);
	}
	return totalColor;
}

vec4 CalcSpotLights()
{
	vec4 totalColor = vec4(0.0, 0.0, 0.0, 1.0);
	for (int i = 0; i < spotLightCount; i++)
	{
		totalColor += CalcSpotLight(spotLights[i], pointLightCount + i);
	}
	return totalColor;
}

// END ---- Phong lighting model ----


// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

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

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

// ---------------------------------------------------------------------------------------------------
// The following code (from Unreal Engine 4's paper) shows how to filter the environment map
// for different roughnesses. This is mean to be computed offline and stored in cube map mips,
// so turning this on online will cause poor performance
float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );
	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
	vec3 TangentX = normalize( cross( UpVector, N ) );
	vec3 TangentY = cross( N, TangentX );
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float TotalWeight = 0.0;

vec3 PrefilterEnvMap(float Roughness, vec3 R)
{
	vec3 N = R;
	vec3 V = R;
	vec3 PrefilteredColor = vec3(0.0);
	int NumSamples = 1024;
	for(int i = 0; i < NumSamples; i++)
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
		vec3 L = 2 * dot(V, H) * H - V;
		float NoL = clamp(dot(N, L), 0.0, 1.0);
		if (NoL > 0)
		{
			PrefilteredColor += texture(u_EnvRadianceTex, L).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

// ---------------------------------------------------------------------------------------------------

vec3 RotateVectorAboutY(float angle, vec3 vec)
{
    angle = radians(angle);
    mat3x3 rotationMatrix ={vec3(cos(angle),0.0,sin(angle)),
                            vec3(0.0,1.0,0.0),
                            vec3(-sin(angle),0.0,cos(angle))};
    return rotationMatrix * vec;
}

vec3 Lighting(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = -u_DirectionalLights.Direction;
		vec3 Lradiance = u_DirectionalLights.Radiance * u_DirectionalLights.Multiplier;
		vec3 Lh = normalize(Li + m_Params.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, m_Params.View)));
		float D = ndfGGX(cosLh, m_Params.Roughness);
		float G = gaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);

		m_Params.ShadowFactor = CalcDirectionalShadowFactor();
		m_Params.PointLights = CalcPointLights();
		m_Params.SpotLights = CalcSpotLights();

		// result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
		result += ((1.0 - m_Params.ShadowFactor) * (diffuseBRDF + specularBRDF)) * Lradiance * cosLi;
		result += m_Params.PointLights.rgb;
		result += m_Params.SpotLights.rgb;
	}

	result *= u_DirectionalLights.Multiplier;
	return result;
}

vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;

	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
	float NoV = clamp(m_Params.NdotV, 0.0, 1.0);
	vec3 R = 2.0 * dot(m_Params.View, m_Params.Normal) * m_Params.Normal - m_Params.View;
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_MaterialUniforms.EnvMapRotation, Lr), m_Params.Roughness * envRadianceTexLevels).rgb;

	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
	vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
	return vec4(linOut, srgbIn.w);
}


void main()
{
	// Standard PBR inputs
	// m_Params.Albedo    = u_MaterialUniforms.AlbedoTexToggle > 0.5 ? texture(u_AlbedoTexture, vs_Input.TexCoord * u_TilingFactor).rgb : u_MaterialUniforms.AlbedoColor;
	m_Params.Albedo    = u_MaterialUniforms.AlbedoTexToggle > 0.5 ?
		texture(u_AlbedoTexture, vs_Input.TexCoord * u_TilingFactor).rgb * u_MaterialUniforms.AlbedoColor :
		u_MaterialUniforms.AlbedoColor;
	m_Params.Metalness = u_MaterialUniforms.MetalnessTexToggle > 0.5 ? texture(u_MetalnessTexture, vs_Input.TexCoord * u_TilingFactor).r : u_MaterialUniforms.Metalness;
	m_Params.Roughness = u_MaterialUniforms.RoughnessTexToggle > 0.5 ? texture(u_RoughnessTexture, vs_Input.TexCoord * u_TilingFactor).r : u_MaterialUniforms.Roughness;
    m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight
	m_Params.Emissive  = u_MaterialUniforms.EmissiveTexToggle > 0.5 ? SRGBtoLINEAR(texture(u_EmissiveTexture, vs_Input.TexCoord * u_TilingFactor)).rgb : vec3(u_MaterialUniforms.Emissive);
	m_Params.AO        = u_MaterialUniforms.AOTexToggle > 0.5 ? texture(u_AOTexture, vs_Input.TexCoord * u_TilingFactor).r : u_MaterialUniforms.AO;

	// Handle Albedo texture transparency
	if(u_MaterialUniforms.AlbedoTexToggle > 0.5 && texture(u_AlbedoTexture, vs_Input.TexCoord * u_TilingFactor).a < 0.1) {
		discard;
	}

	// Normals (either from vertex or map)
	m_Params.Normal = normalize(vs_Input.Normal);
	if (u_MaterialUniforms.NormalTexToggle > 0.5)
	{
		m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, vs_Input.TexCoord * u_TilingFactor).rgb - 1.0);
		m_Params.Normal = normalize(vs_Input.WorldNormals * m_Params.Normal);
	}

	m_Params.View = normalize(u_CameraPosition - vs_Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);

	// Specular reflection vector
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	vec3 lightContribution = Lighting(F0); // vec3(0.0) / Lighting(F0);
	vec3 iblContribution = IBL(F0, Lr);
	iblContribution.rgb *= m_Params.AO;

	color = vec4(lightContribution + iblContribution, 1.0);
	color.rgb += m_Params.Emissive;
	color.rgb *= u_Exposure; // originally used in Shaders/Hazel/SceneComposite

	if (u_WireframeMode.Enabled)
	{
		color = u_WireframeMode.LineColor;
	}
}
