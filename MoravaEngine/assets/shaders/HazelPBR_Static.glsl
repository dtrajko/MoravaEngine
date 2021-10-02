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
	Output.WorldPosition = vec3(u_Transform * vec4(a_Position, 1.0));
    Output.Normal = mat3(u_Transform) * a_Normal;
	Output.TexCoord = a_TexCoord;//vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	Output.WorldNormals = mat3(u_Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Transform);
	Output.Binormal = a_Binormal;

	gl_Position = u_ViewProjectionMatrix * u_Transform * vec4(a_Position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int LightCount = 1;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct Light {
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
};

layout (location = 0) in VertexOutput Input;

layout(location = 0) out vec4 color;

layout (std140, binding = 1) uniform Environment
{
	Light lights;
	vec3 u_CameraPosition; // Offset = 32
};

// PBR texture inputs
layout (binding = 2) uniform sampler2D u_AlbedoTexture;
layout (binding = 3) uniform sampler2D u_NormalTexture;
layout (binding = 4) uniform sampler2D u_MetalnessTexture;
layout (binding = 5) uniform sampler2D u_RoughnessTexture;

// Environment maps
layout (set = 1, binding = 0) uniform samplerCube u_EnvRadianceTex;
layout (set = 1, binding = 1) uniform samplerCube u_EnvIrradianceTex;

// BRDF LUT
layout (set = 1, binding = 2) uniform sampler2D u_BRDFLUTTexture;

layout (push_constant) uniform Material
{
	layout (offset = 64) vec3 AlbedoColor;
	float Metalness;
	float Roughness;

	float EnvMapRotation;

	// Toggles
	float RadiancePrefilter;
	float AlbedoTexToggle;
	float NormalTexToggle;
	float MetalnessTexToggle;
	float RoughnessTexToggle;
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

vec3 Lighting(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = lights.Direction;
		vec3 Lradiance = lights.Radiance * lights.Multiplier;
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

vec3 LightingTemp(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = lights.Direction; // vec3(-0.5, 0.5, 0.5); // 
		vec3 Lradiance = vec3(1.0, 1.0, 1.0) * 2.0; //  lights.Radiance * lights.Multiplier;
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
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_MaterialUniforms.EnvMapRotation, Lr), m_Params.Roughness * envRadianceTexLevels).rgb;

	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, 1.0 - m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

void main()
{
	//	Standard PBR inputs

	// Temporary (values are not updated through the uniform buffer)
	float u_MaterialUniforms_AlbedoTexToggle = 1.0;
	float u_MaterialUniforms_NormalTexToggle = 1.0;
	float u_MaterialUniforms_MetalnessTexToggle = 1.0;
	float u_MaterialUniforms_RoughnessTexToggle = 1.0;

	m_Params.Albedo = u_MaterialUniforms_AlbedoTexToggle > 0.5 ? texture(u_AlbedoTexture, Input.TexCoord).rgb : u_MaterialUniforms.AlbedoColor;
	m_Params.Metalness = u_MaterialUniforms_MetalnessTexToggle > 0.5 ? texture(u_MetalnessTexture, Input.TexCoord).r : u_MaterialUniforms.Metalness;
	m_Params.Roughness = u_MaterialUniforms_RoughnessTexToggle > 0.5 ?  texture(u_RoughnessTexture, Input.TexCoord).r : u_MaterialUniforms.Roughness;
	m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	m_Params.Normal = normalize(Input.Normal);
	if (u_MaterialUniforms_NormalTexToggle > 0.5)
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

	vec3 lightContribution = LightingTemp(F0);
	vec3 iblContribution = IBL(F0, Lr);

	// color = vec4(lightContribution, 1.0);
	color = vec4(lightContribution + iblContribution, 1.0);

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
