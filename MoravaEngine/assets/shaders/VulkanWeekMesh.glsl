// Vulkan Triangle shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout (location = 0) out vec3 v_Normal;
layout (location = 1) out vec2 v_TexCoord;
layout (location = 2) out mat3 v_WorldNormals;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProj;
};

layout (push_constant) uniform Transform
{
	mat4 u_Transform;
};

void main()
{
	gl_Position = u_ViewProj * u_Transform * vec4(a_Position, 1.0);

	v_Normal = a_Normal;
	v_TexCoord = a_TexCoord;
	v_WorldNormals = mat3(u_Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 finalColor;

layout (location = 0) in vec3 v_Normal;
layout (location = 1) in vec2 v_TexCoord;
layout (location = 2) in mat3 v_WorldNormals;

layout (binding = 1) uniform sampler2D u_AlbedoTexture;
layout (binding = 2) uniform sampler2D u_NormalTexture;

layout (push_constant) uniform Material
{
	layout(offset = 64) vec4 u_Color;
};

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

void main()
{
	m_Params.Albedo = texture(u_AlbedoTexture, v_TexCoord).rgb;

	// Normals (either from vertex or map)
	// m_Params.Normal = normalize(v_Normal);
	m_Params.Normal = normalize(2.0 * texture(u_NormalTexture, v_TexCoord).rgb - 1.0);
	m_Params.Normal = normalize(v_WorldNormals * m_Params.Normal);

	float ambient = 0.2;
	vec3 lightDir = vec3(-1.0, 1.0, 0.0);
	float intensity = clamp(dot(lightDir, m_Params.Normal), ambient, 1.0);

	finalColor = vec4(m_Params.Albedo, 1.0) * u_Color;
	finalColor.rgb *= intensity;
}
