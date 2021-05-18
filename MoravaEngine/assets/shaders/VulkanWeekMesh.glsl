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

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProj;
};

layout (std140, binding = 1) uniform Transform
{
	mat4 u_Transform;
};

layout(push_constant) uniform PushConstants
{
	mat4 pc_Transform;
} push_constants;

void main()
{
	// gl_Position = vec4(a_Position, 1.0);
	gl_Position = u_ViewProj * u_Transform * vec4(a_Position, 1.0);
	v_TexCoord = a_TexCoord;
	v_Normal = a_Normal;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 finalColor;

layout (location = 0) in vec3 v_Normal;
layout (location = 1) in vec2 v_TexCoord;

layout(push_constant) uniform PushConstants
{
	vec4 pc_Color;
} push_constants;

void main()
{
	float ambient = 0.2;
	vec3 lightDir = vec3(-0.4, 0.8, 0.4);
	float intensity = clamp(dot(lightDir, v_Normal), ambient, 1.0);

	finalColor = vec4(1.0, 0.0, 1.0, 1.0);
	// finalColor.rgb = v_Normal * 0.5 + 0.5;
	finalColor.rgb *= intensity;
}
