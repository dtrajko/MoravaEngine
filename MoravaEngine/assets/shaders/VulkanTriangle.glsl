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

layout (binding = 0) uniform Camera
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

layout (location = 0) in vec3 a_Normal;

layout (binding = 1) uniform sampler2D u_AlbedoTexture;
layout (binding = 2) uniform sampler2D u_NormalTexture;

layout (location = 1) in vec2 v_TexCoord;
layout (location = 2) in mat3 v_WorldNormals;

layout (push_constant) uniform Material
{
	layout(offset = 64) vec4 Color;
};

void main()
{
	vec3 normal = normalize(2.0 * texture(u_NormalTexture, v_TexCoord).rgb - 1.0);
	normal = normalize(v_WorldNormals * normal);

	vec3 lightDir = vec3(-0.5, 0.5, -0.5);
	float intensity = clamp(dot(lightDir, normal), 0.1, 1.0);

	vec4 texColor = texture(u_AlbedoTexture, v_TexCoord);

	finalColor = texColor * Color;//vec4(1.0, 0.0, 1.0, 1.0);
	finalColor.rgb *= intensity;
}