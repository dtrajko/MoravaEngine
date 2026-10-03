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
// Descriptor sets (Vulkan), ordered by how often they change (see VulkanShaderH2M::FrameDescriptorSet):
// - set 0, per frame:    Camera, SceneData (camera position, environment rotation), environment maps, BRDF LUT, Lights,
//                        the sun's shadow map and its cascades
// - set 1, per material: the material's texture maps
// - set 2, per object:   bone matrices (HazelPBR_Anim.glsl only)
// Push constants: the transform (vertex stage) and the material values (fragment stage).
#type vertex
#version 450 core

// Skinned version of HazelPBR_Static.glsl (Vulkan). The fragment stage is an exact copy of the static shader,
// so materials, texture slots and the Material Editor work the same for animated meshes.

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

// Set 0, per frame
layout (std140, set = 0, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
};

// Final bone matrices of the current animation frame (MeshH2M::GetBoneTransforms), updated every frame
// Set 2, per object: final bone matrices of the current animation frame (MeshH2M::GetBoneTransforms), updated every frame
const int MAX_BONES = 128;
layout (std140, set = 2, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
};

layout (push_constant) uniform Transform
{
	mat4 u_Transform;
};

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	// Vertices without bone weights (parts of the model that are not attached to the skeleton) keep their shape
	mat4 boneTransform = mat4(1.0);
	if (dot(a_BoneWeights, vec4(1.0)) > 0.0001)
	{
		ivec4 indices = min(a_BoneIndices, ivec4(MAX_BONES - 1));
		boneTransform  = u_BoneTransforms[indices.x] * a_BoneWeights.x;
		boneTransform += u_BoneTransforms[indices.y] * a_BoneWeights.y;
		boneTransform += u_BoneTransforms[indices.z] * a_BoneWeights.z;
		boneTransform += u_BoneTransforms[indices.w] * a_BoneWeights.w;
	}

	mat4 skinnedTransform = u_Transform * boneTransform;

	Output.WorldPosition = vec3(skinnedTransform * vec4(a_Position, 1.0));
	Output.Normal = mat3(skinnedTransform) * a_Normal;
	Output.TexCoord = a_TexCoord;
	// The tangent frame moves with the bones too (the normal map would be lit wrong on moving parts otherwise)
	Output.WorldNormals = mat3(skinnedTransform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Transform);
	Output.Binormal = mat3(boneTransform) * a_Binormal;

	gl_Position = u_ViewProjectionMatrix * skinnedTransform * vec4(a_Position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

// Lights (EnvMapVulkanLights.h): the std140 layouts must match EnvMapVulkanLightsGPU byte for byte
const int MaxPointLights = 16;
const int MaxSpotLights = 16;

struct DirectionalLight
{
	vec3 Direction;  // towards the light
	float Intensity; // 0 when the sun is disabled
	vec3 Color;
	float Padding;
};

struct PointLight
{
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Range;     // the light reaches exactly zero here
};

struct SpotLight
{
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Range;
	vec3 Direction;  // the direction the light travels
	float CosOuter;
	float CosInner;
	float Padding0;  // three floats, not a vec3: a std140 vec3 would be aligned to 16 bytes
	float Padding1;
	float Padding2;
};

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout (location = 0) in VertexOutput Input;

layout(location = 0) out vec4 color;

// Set 0, per frame: shared by every mesh
layout (std140, set = 0, binding = 1) uniform SceneData
{
	vec3 u_CameraPosition;    // offset 0
	float u_EnvMapRotation;   // offset 12: degrees around Y, applied to the environment lookups
};
layout (set = 0, binding = 2) uniform samplerCube u_EnvRadianceTex;
layout (set = 0, binding = 3) uniform samplerCube u_EnvIrradianceTex;
layout (set = 0, binding = 4) uniform sampler2D u_BRDFLUTTexture;
layout (std140, set = 0, binding = 5) uniform Lights
{
	DirectionalLight u_Sun;                    // offset 0 (32 bytes)
	int u_PointLightCount;                     // offset 32
	int u_SpotLightCount;                      // offset 36
	ivec2 u_LightsPadding;                     // offset 40
	PointLight u_PointLights[MaxPointLights];  // offset 48 (32 bytes each)
	SpotLight u_SpotLights[MaxSpotLights];     // offset 560 (64 bytes each)
};
// The sun's cascaded shadow map (EnvMapVulkanShadows.h): a layer per cascade, compared in the sampler (1 lit, 0 shadow)
layout (set = 0, binding = 6) uniform sampler2DArrayShadow u_ShadowMap;
layout (std140, set = 0, binding = 7) uniform Shadows
{
	mat4 u_CascadeViewProjection[4]; // offset 0: world -> shadow map clip space, per cascade
	vec4 u_CascadeSplits;            // offset 256: where each cascade ends (distance along the camera's view direction)
	vec4 u_CascadeTexelSizes;        // offset 272: world size of a shadow map texel, per cascade
	vec3 u_CameraForward;            // offset 288
	float u_ShadowsEnabled;          // offset 300: 0 = no shadows this frame
	float u_NormalBias;              // offset 304: in texels
	float u_ShadowSoftness;          // offset 308: PCF tap spacing in texels
	float u_ShowCascades;            // offset 312: 1 = tint the scene by cascade
	float u_ShadowMapTexelSize;      // offset 316: 1 / resolution (texture coordinate units)
};

// Set 1, per material: PBR texture maps
layout (set = 1, binding = 0) uniform sampler2D u_AlbedoTexture;
layout (set = 1, binding = 1) uniform sampler2D u_NormalTexture;
layout (set = 1, binding = 2) uniform sampler2D u_MetalnessTexture;
layout (set = 1, binding = 3) uniform sampler2D u_RoughnessTexture;
layout (set = 1, binding = 4) uniform sampler2D u_EmissiveTexture;
layout (set = 1, binding = 5) uniform sampler2D u_AOTexture;

layout (push_constant) uniform Material
{
	layout (offset = 64) vec3 AlbedoColor;
	float Metalness;
	float Roughness;

	// Toggles
	float RadiancePrefilter;
	float AlbedoTexToggle;
	float NormalTexToggle;
	float MetalnessTexToggle;
	float RoughnessTexToggle;

	// Offsets 104..123 (the whole block ends at 124 bytes; 128 is the minimum maxPushConstantsSize guaranteed by Vulkan)
	float TilingFactor;       // texture coordinate scale (0 is treated as 1)
	float EmissiveTexToggle;
	float AOTexToggle;
	float EmissiveIntensity;  // multiplies the emissive map (HDR values above 1 are fine)
	float MetalRoughPacked;   // 1: glTF metallicRoughness map (roughness in G, metalness in B); 0: both maps read from R
} u_MaterialUniforms;

struct PBRParameters
{
	vec3 Albedo;
	vec3 Normal;
	float Metalness;
	float Roughness;

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
			// PrefilteredColor += texture(u_EnvRadianceTex, L).rgb * NoL;
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

// Cook-Torrance BRDF for one light. L points from the surface towards the light; radiance is the light that arrives
// at the surface (attenuation and cone already applied).
vec3 EvaluateBRDF(vec3 F0, vec3 L, vec3 radiance)
{
	float cosLi = max(0.0, dot(m_Params.Normal, L));
	if (cosLi <= 0.0)
	{
		return vec3(0.0);
	}
	vec3 Lh = normalize(L + m_Params.View);
	float cosLh = max(0.0, dot(m_Params.Normal, Lh));

	vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, m_Params.View)));
	float D = ndfGGX(cosLh, m_Params.Roughness);
	float G = gaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseBRDF = kd * m_Params.Albedo;
	vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);

	return (diffuseBRDF + specularBRDF) * radiance * cosLi;
}

// Inverse-square falloff, windowed to reach exactly zero at the light's range (Karis 2013, "Real Shading in Unreal Engine 4").
// The distance is clamped to 1 cm, so a surface at the light's position doesn't get infinite light.
float DistanceAttenuation(float distance, float range)
{
	float ratio = distance / range;
	float ratio2 = ratio * ratio;
	float window = clamp(1.0 - ratio2 * ratio2, 0.0, 1.0);
	return window * window / max(distance * distance, 0.0001);
}

// The cascade a point at this view distance belongs to (the last one beyond the shadow distance)
int GetShadowCascade(float viewDistance)
{
	for (int i = 0; i < 3; i++)
	{
		if (viewDistance < u_CascadeSplits[i])
		{
			return i;
		}
	}
	return 3;
}

// How much of the sun reaches the point in one cascade: 1 lit, 0 in shadow
float SampleShadowCascade(int cascade, vec3 worldPosition, vec3 geometryNormal)
{
	// Normal offset: look the map up from a point slightly off the surface (more on surfaces that turn away from the
	// sun, and in proportion to the cascade's texel size), so a lit surface doesn't shadow itself ("shadow acne")
	float cosTheta = clamp(dot(geometryNormal, u_Sun.Direction), 0.0, 1.0);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 position = worldPosition + geometryNormal * (u_NormalBias * u_CascadeTexelSizes[cascade] * (0.25 + sinTheta));

	vec4 clip = u_CascadeViewProjection[cascade] * vec4(position, 1.0);
	vec3 shadowCoord = clip.xyz / clip.w;
	vec2 uv = shadowCoord.xy * 0.5 + 0.5;
	if (shadowCoord.z >= 1.0)
	{
		return 1.0; // beyond the cascade's far plane: nothing can shadow it
	}

	// PCF: a 5x5 grid of comparisons, each one already a blend of 2x2 texels (the sampler's linear filter)
	float lit = 0.0;
	vec2 pcfStep = vec2(u_ShadowSoftness * u_ShadowMapTexelSize);
	for (int y = -2; y <= 2; y++)
	{
		for (int x = -2; x <= 2; x++)
		{
			lit += texture(u_ShadowMap, vec4(uv + vec2(x, y) * pcfStep, float(cascade), shadowCoord.z));
		}
	}
	return lit / 25.0;
}

// The sun's shadow at the fragment: 1 lit, 0 in shadow. Near the end of a cascade it blends into the next one (no
// visible seam), and the shadows fade out before the shadow distance (no hard end).
float SunShadow(vec3 worldPosition, vec3 geometryNormal)
{
	if (u_ShadowsEnabled < 0.5)
	{
		return 1.0;
	}
	float viewDistance = dot(worldPosition - u_CameraPosition, u_CameraForward);
	float shadowDistance = u_CascadeSplits[3];
	if (viewDistance >= shadowDistance)
	{
		return 1.0;
	}

	int cascade = GetShadowCascade(viewDistance);
	float shadow = SampleShadowCascade(cascade, worldPosition, geometryNormal);

	// Blend zone: the last tenth of the cascade (the next cascade covers it too, see ComputeShadowCascades)
	if (cascade < 3)
	{
		float start = cascade == 0 ? 0.0 : u_CascadeSplits[cascade - 1];
		float blendStart = u_CascadeSplits[cascade] - 0.1 * (u_CascadeSplits[cascade] - start);
		if (viewDistance > blendStart)
		{
			float t = (viewDistance - blendStart) / (u_CascadeSplits[cascade] - blendStart);
			shadow = mix(shadow, SampleShadowCascade(cascade + 1, worldPosition, geometryNormal), t);
		}
	}

	// Fade out over the last tenth of the shadow distance
	float fadeStart = shadowDistance * 0.9;
	return mix(shadow, 1.0, clamp((viewDistance - fadeStart) / (shadowDistance - fadeStart), 0.0, 1.0));
}

// Show Cascades: red, green, blue, yellow
vec3 CascadeDebugColor(vec3 worldPosition)
{
	const vec3 colors[4] = vec3[4](vec3(1.0, 0.3, 0.3), vec3(0.3, 1.0, 0.3), vec3(0.3, 0.5, 1.0), vec3(1.0, 1.0, 0.3));
	float viewDistance = dot(worldPosition - u_CameraPosition, u_CameraForward);
	return viewDistance < u_CascadeSplits[3] ? colors[GetShadowCascade(viewDistance)] : vec3(1.0);
}

// Direct light from the sun, the point lights and the spot lights (lists packed by EnvMapVulkanLightEnvironment::Pack)
vec3 Lighting(vec3 F0)
{
	// The sun, through its shadow map (sampled only where the sun can reach the surface at all)
	vec3 result = vec3(0.0);
	if (u_Sun.Intensity > 0.0 && dot(m_Params.Normal, u_Sun.Direction) > 0.0)
	{
		float shadow = SunShadow(Input.WorldPosition, normalize(Input.Normal));
		result = EvaluateBRDF(F0, u_Sun.Direction, u_Sun.Color * u_Sun.Intensity * shadow);
	}

	int pointLightCount = min(u_PointLightCount, MaxPointLights);
	for (int i = 0; i < pointLightCount; i++)
	{
		vec3 toLight = u_PointLights[i].Position - Input.WorldPosition;
		float distance = length(toLight);
		if (distance >= u_PointLights[i].Range)
		{
			continue;
		}
		vec3 L = toLight / max(distance, Epsilon);
		float attenuation = DistanceAttenuation(distance, u_PointLights[i].Range);
		result += EvaluateBRDF(F0, L, u_PointLights[i].Color * u_PointLights[i].Intensity * attenuation);
	}

	int spotLightCount = min(u_SpotLightCount, MaxSpotLights);
	for (int i = 0; i < spotLightCount; i++)
	{
		vec3 toLight = u_SpotLights[i].Position - Input.WorldPosition;
		float distance = length(toLight);
		if (distance >= u_SpotLights[i].Range)
		{
			continue;
		}
		vec3 L = toLight / max(distance, Epsilon);
		// Full intensity inside the inner cone, a smooth fade to zero at the outer cone
		float cone = smoothstep(u_SpotLights[i].CosOuter, u_SpotLights[i].CosInner, dot(-L, u_SpotLights[i].Direction));
		if (cone <= 0.0)
		{
			continue;
		}
		float attenuation = DistanceAttenuation(distance, u_SpotLights[i].Range);
		result += EvaluateBRDF(F0, L, u_SpotLights[i].Color * u_SpotLights[i].Intensity * attenuation * cone);
	}

	return result;
}

vec3 IBL(vec3 F0, vec3 Lr)
{
	// The environment rotation applies to the diffuse lighting too (not only to the reflections), so both match the skybox
	vec3 irradiance = texture(u_EnvIrradianceTex, RotateVectorAboutY(u_EnvMapRotation, m_Params.Normal)).rgb;
	vec3 F = fresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;

	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
	float NoV = clamp(m_Params.NdotV, 0.0, 1.0);
	vec3 R = 2.0 * dot(m_Params.View, m_Params.Normal) * m_Params.Normal - m_Params.View;
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, Lr), m_Params.Roughness * envRadianceTexLevels).rgb;

	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

void main()
{
	//	Standard PBR inputs

	// Texture toggles and values come from the material (push constants, editable in the Material Editor)
	float tiling = u_MaterialUniforms.TilingFactor > 0.0 ? u_MaterialUniforms.TilingFactor : 1.0;
	vec2 texCoord = Input.TexCoord * tiling;
	bool packed = u_MaterialUniforms.MetalRoughPacked > 0.5;

	m_Params.Albedo = u_MaterialUniforms.AlbedoTexToggle > 0.5 ? texture(u_AlbedoTexture, texCoord).rgb : u_MaterialUniforms.AlbedoColor;
	m_Params.Metalness = u_MaterialUniforms.MetalnessTexToggle > 0.5 ? (packed ? texture(u_MetalnessTexture, texCoord).b : texture(u_MetalnessTexture, texCoord).r) : u_MaterialUniforms.Metalness;
	m_Params.Roughness = u_MaterialUniforms.RoughnessTexToggle > 0.5 ? (packed ? texture(u_RoughnessTexture, texCoord).g : texture(u_RoughnessTexture, texCoord).r) : u_MaterialUniforms.Roughness;
	m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	m_Params.Normal = normalize(Input.Normal);
	if (u_MaterialUniforms.NormalTexToggle > 0.5)
	{
		m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, texCoord).rgb - 1.0);
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
	}

	m_Params.View = normalize(u_CameraPosition - Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);

	// Specular reflection vector
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;

	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	vec3 lightContribution = Lighting(F0);
	vec3 iblContribution = IBL(F0, Lr);

	// Ambient occlusion darkens only the indirect (environment) light; the map is linear data in R
	float ao = u_MaterialUniforms.AOTexToggle > 0.5 ? texture(u_AOTexture, texCoord).r : 1.0;

	// Emission is added on top of the lit surface (the map is sRGB color data, decoded by the sampler)
	vec3 emissive = u_MaterialUniforms.EmissiveTexToggle > 0.5 ? texture(u_EmissiveTexture, texCoord).rgb * u_MaterialUniforms.EmissiveIntensity : vec3(0.0);

	color = vec4(lightContribution + iblContribution * ao + emissive, 1.0);
	if (u_ShowCascades > 0.5 && u_ShadowsEnabled > 0.5)
	{
		color.rgb *= CascadeDebugColor(Input.WorldPosition);
	}

	// color = vec4(Input.WorldPosition, 1.0);
	// color = texture(u_RoughnessTexture, Input.TexCoord);
	// color = vec4(F0, 1.0);
	// color = vec4(m_Params.View, 1.0);

	// vec3 albedo = texture(u_AlbedoTexture, Input.TexCoord).rgb;
	// color = vec4(albedo, 1);

	/**** BEGIN main() from VulkanWeekMesh ****/
	// m_Params.Albedo = texture(u_AlbedoTexture, Input.TexCoord).rgb;
	// 
	// // Normals (either from vertex or map)
	// m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, Input.TexCoord).rgb - 1.0);
	// m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
	// 
	// float ambient = 0.2;
	// vec3 lightDir = vec3(-1.0, 1.0, 0.0);
	// float intensity = clamp(dot(lightDir, m_Params.Normal), ambient, 1.0);
	// 
	// color = vec4(m_Params.Albedo, 1.0);
	// color.rgb *= intensity;

	/**** END main() from VulkanWeekMesh ****/
}
