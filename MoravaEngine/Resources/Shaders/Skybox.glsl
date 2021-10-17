// Skybox shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
	mat4 u_InverseViewProjection;
};

layout (location = 0) out vec3 v_Position;

void main()
{
	vec4 position = vec4(a_Position.xy, 1.0, 1.0);
	gl_Position = position;

	v_Position = (u_InverseViewProjection * position).xyz;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 finalColor;
// layout(location = 1) out vec4 o_Bloom;

layout (binding = 1) uniform samplerCube u_Texture;

layout (push_constant) uniform Uniforms
{
	float TextureLod;
} u_Uniforms;

layout (location = 0) in vec3 v_Position;

void main()
{
	finalColor = textureLod(u_Texture, v_Position, u_Uniforms.TextureLod);
	// o_Bloom = vec4(0.0);
}
