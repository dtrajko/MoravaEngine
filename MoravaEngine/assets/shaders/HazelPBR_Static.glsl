// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated. 
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michał Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
	mat4 u_InverseViewProjectionMatrix;
};

layout (std140, binding = 1) uniform ShadowData
{
	mat4 u_LightMatrixCascade0;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;

	vec4 ShadowMapCoords;
	vec4 ShadowMapCoordsBiased;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.WorldPosition = vec3(u_Renderer.Transform * vec4(a_Position, 1.0));
    Output.Normal = mat3(u_Renderer.Transform) * a_Normal;
	Output.TexCoord = a_TexCoord;//vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	Output.WorldNormals = mat3(u_Renderer.Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Renderer.Transform);
	Output.Binormal = a_Binormal;

	Output.ShadowMapCoords = u_LightMatrixCascade0 * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoordsBiased = u_LightMatrixCascade0 * vec4(Output.WorldPosition, 1.0);

	gl_Position = u_ViewProjectionMatrix * u_Renderer.Transform * vec4(a_Position, 1.0);
}

#type fragment
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

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 ShadowMapCoords;
	vec4 ShadowMapCoordsBiased;
};

layout (location = 0) in VertexOutput Input;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 o_BloomColor;

layout (std140, binding = 2) uniform SceneData
{
	DirectionalLight u_DirectionalLights;
	vec3 u_CameraPosition; // Offset = 32
	bool u_HasEnvironmentMap;
};

layout (std140, binding = 3) uniform RendererData
{
	uniform bool u_ShowCascades;
	uniform bool u_SoftShadows;
	uniform float u_LightSize;
	uniform float u_MaxShadowDistance;
	uniform float u_ShadowFade;
	uniform bool u_CascadeFading;
	uniform float u_CascadeTransitionFade;
};

// PBR texture inputs
layout (set = 0, binding = 4) uniform sampler2D u_AlbedoTexture;
layout (set = 0, binding = 5) uniform sampler2D u_NormalTexture;
layout (set = 0, binding = 6) uniform sampler2D u_MetalnessTexture;
layout (set = 0, binding = 7) uniform sampler2D u_RoughnessTexture;

// Environment maps
layout (set = 1, binding = 8) uniform samplerCube u_EnvRadianceTex;
layout (set = 1, binding = 9) uniform samplerCube u_EnvIrradianceTex;

// BRDF LUT
layout (set = 1, binding = 10) uniform sampler2D u_BRDFLUTTexture;

// Shadow maps
layout (set = 1, binding = 11) uniform sampler2D u_ShadowMapTexture;

layout (push_constant) uniform Material
{
	layout (offset = 64) vec3 AlbedoColor;
	float Metalness;
	float Roughness;

	float EnvMapRotation;

	bool UseNormalMap;
} u_MaterialUniforms;

struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
};

PBRParameters m_Params;

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
			//PrefilteredColor += texture(u_EnvRadianceTex, L).rgb * NoL;
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
		vec3 Li = u_DirectionalLights.Direction;
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

		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
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
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_MaterialUniforms.EnvMapRotation, Lr), (m_Params.Roughness) * envRadianceTexLevels).rgb;
	
	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	
	return kd * diffuseIBL + specularIBL;
}

/////////////////////////////////////////////
// PCSS
/////////////////////////////////////////////

uint CascadeIndex = 0;
float ShadowFade = 1.0;

float GetShadowBias()
{
	const float MINIMUM_SHADOW_BIAS = 0.002;
	float bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(m_Params.Normal, u_DirectionalLights.Direction)), MINIMUM_SHADOW_BIAS);
	return bias;
}

float HardShadows_DirectionalLight(sampler2D shadowMap, vec3 shadowCoords)
{
	float depth = shadowCoords.z;
#ifdef OPENGL
	depth = depth * 0.5 + 0.5; // scale bias for OpenGL depth value
#endif
	float bias = GetShadowBias();
	float shadowMapDepth = texture(shadowMap, shadowCoords.xy * 0.5 + 0.5).x;
	return step(depth, shadowMapDepth + bias) * ShadowFade;
}

// Penumbra

// this search area estimation comes from the following article: 
// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float uvLightSize, float receiverDistance)
{
	const float NEAR = 0.1;
	return uvLightSize * (receiverDistance - NEAR) / u_CameraPosition.z;
}

float SearchRegionRadiusUV(float zWorld)
{
	const float light_zNear = 0.0; // 0.01 gives artifacts? maybe because of ortho proj?
	const float lightRadiusUV = 0.05;
    return lightRadiusUV * (zWorld - light_zNear) / zWorld;
}

const vec2 PoissonDistribution[64] = vec2[](
	vec2(-0.884081, 0.124488),
	vec2(-0.714377, 0.027940),
	vec2(-0.747945, 0.227922),
	vec2(-0.939609, 0.243634),
	vec2(-0.985465, 0.045534),
	vec2(-0.861367, -0.136222),
	vec2(-0.881934, 0.396908),
	vec2(-0.466938, 0.014526),
	vec2(-0.558207, 0.212662),
	vec2(-0.578447, -0.095822),
	vec2(-0.740266, -0.095631),
	vec2(-0.751681, 0.472604),
	vec2(-0.553147, -0.243177),
	vec2(-0.674762, -0.330730),
	vec2(-0.402765, -0.122087),
	vec2(-0.319776, -0.312166),
	vec2(-0.413923, -0.439757),
	vec2(-0.979153, -0.201245),
	vec2(-0.865579, -0.288695),
	vec2(-0.243704, -0.186378),
	vec2(-0.294920, -0.055748),
	vec2(-0.604452, -0.544251),
	vec2(-0.418056, -0.587679),
	vec2(-0.549156, -0.415877),
	vec2(-0.238080, -0.611761),
	vec2(-0.267004, -0.459702),
	vec2(-0.100006, -0.229116),
	vec2(-0.101928, -0.380382),
	vec2(-0.681467, -0.700773),
	vec2(-0.763488, -0.543386),
	vec2(-0.549030, -0.750749),
	vec2(-0.809045, -0.408738),
	vec2(-0.388134, -0.773448),
	vec2(-0.429392, -0.894892),
	vec2(-0.131597, 0.065058),
	vec2(-0.275002, 0.102922),
	vec2(-0.106117, -0.068327),
	vec2(-0.294586, -0.891515),
	vec2(-0.629418, 0.379387),
	vec2(-0.407257, 0.339748),
	vec2(0.071650, -0.384284),
	vec2(0.022018, -0.263793),
	vec2(0.003879, -0.136073),
	vec2(-0.137533, -0.767844),
	vec2(-0.050874, -0.906068),
	vec2(0.114133, -0.070053),
	vec2(0.163314, -0.217231),
	vec2(-0.100262, -0.587992),
	vec2(-0.004942, 0.125368),
	vec2(0.035302, -0.619310),
	vec2(0.195646, -0.459022),
	vec2(0.303969, -0.346362),
	vec2(-0.678118, 0.685099),
	vec2(-0.628418, 0.507978),
	vec2(-0.508473, 0.458753),
	vec2(0.032134, -0.782030),
	vec2(0.122595, 0.280353),
	vec2(-0.043643, 0.312119),
	vec2(0.132993, 0.085170),
	vec2(-0.192106, 0.285848),
	vec2(0.183621, -0.713242),
	vec2(0.265220, -0.596716),
	vec2(-0.009628, -0.483058),
	vec2(-0.018516, 0.435703)
);

const vec2 poissonDisk[16] = vec2[](
 vec2( -0.94201624, -0.39906216 ),
 vec2( 0.94558609, -0.76890725 ),
 vec2( -0.094184101, -0.92938870 ),
 vec2( 0.34495938, 0.29387760 ),
 vec2( -0.91588581, 0.45771432 ),
 vec2( -0.81544232, -0.87912464 ),
 vec2( -0.38277543, 0.27676845 ),
 vec2( 0.97484398, 0.75648379 ),
 vec2( 0.44323325, -0.97511554 ),
 vec2( 0.53742981, -0.47373420 ),
 vec2( -0.26496911, -0.41893023 ),
 vec2( 0.79197514, 0.19090188 ),
 vec2( -0.24188840, 0.99706507 ),
 vec2( -0.81409955, 0.91437590 ),
 vec2( 0.19984126, 0.78641367 ),
 vec2( 0.14383161, -0.14100790 )
); 

vec2 SamplePoisson(int index)
{
   return PoissonDistribution[index % 64];
}

float FindBlockerDistance_DirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvLightSize)
{
	float bias = GetShadowBias();

	int numBlockerSearchSamples = 64;
	int blockers = 0;
	float avgBlockerDistance = 0;
	
	float searchWidth = SearchRegionRadiusUV(shadowCoords.z);
	for (int i = 0; i < numBlockerSearchSamples; i++)
	{
		float z = textureLod(shadowMap, (shadowCoords.xy * 0.5 + 0.5) + SamplePoisson(i) * searchWidth, 0).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / float(blockers);

	return -1;
}

float PCF_DirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();
	int numPCFSamples = 64;
	float sum = 0;
	for (int i = 0; i < numPCFSamples; i++)
	{
		vec2 offset = SamplePoisson(i) * uvRadius;
		float z = textureLod(shadowMap, (shadowCoords.xy * 0.5 + 0.5) + offset, 0).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / numPCFSamples;
}

float NV_PCF_DirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();
	float sum = 0;
	for (int i = 0; i < 16; i++)
	{
		vec2 offset = poissonDisk[i] * uvRadius;
		float z = textureLod(shadowMap, (shadowCoords.xy * 0.5 + 0.5) + offset, 0).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / 16.0f;
}

float PCSS_DirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvLightSize)
{
	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowMap, shadowCoords, uvLightSize);
	if (blockerDistance == -1) // No occlusion
		return 1.0f;

	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	float NEAR = 0.01; // Should this value be tweakable?
	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z; // Do we need to divide by shadowCoords.z?
	uvRadius = min(uvRadius, 0.002f);
	return PCF_DirectionalLight(shadowMap, shadowCoords, uvRadius) * ShadowFade;
}

/////////////////////////////////////////////

void main()
{
	// Standard PBR inputs
	m_Params.Albedo = texture(u_AlbedoTexture, Input.TexCoord).rgb * u_MaterialUniforms.AlbedoColor; 
	m_Params.Metalness = texture(u_MetalnessTexture, Input.TexCoord).r * u_MaterialUniforms.Metalness;
	m_Params.Roughness = texture(u_RoughnessTexture, Input.TexCoord).r * u_MaterialUniforms.Roughness;
    m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	m_Params.Normal = normalize(Input.Normal);
	if (u_MaterialUniforms.UseNormalMap)
	{
		m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, Input.TexCoord).rgb - 1.0);
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
	}
	
	m_Params.View = normalize(u_CameraPosition - Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
		
	// Specular reflection vector
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);
	
	vec3 shadowMapCoords = (Input.ShadowMapCoords.xyz / Input.ShadowMapCoords.w);
	vec3 biasedShadowMapCoords = (Input.ShadowMapCoordsBiased.xyz / Input.ShadowMapCoordsBiased.w);
	float shadowAmount = HardShadows_DirectionalLight(u_ShadowMapTexture, shadowMapCoords);

	float lightSize = 0.5;
	shadowAmount = PCSS_DirectionalLight(u_ShadowMapTexture, shadowMapCoords, lightSize);

	vec3 lightContribution = Lighting(F0) * shadowAmount;
	vec3 iblContribution = IBL(F0, Lr);

	color = vec4(iblContribution + lightContribution, 1.0);

	o_BloomColor = vec4(1.0, 0.0, 1.0, 1.0);
}
