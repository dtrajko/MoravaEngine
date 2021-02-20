// Skybox shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_InverseVP;
};

layout (location = 0) out vec3 v_Position;

void main()
{
	vec4 position = vec4(a_Position.xy, 1.0, 1.0);
	gl_Position = position;

	v_Position = (u_InverseVP * position).xyz;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 finalColor;

layout (binding = 0) uniform samplerCube u_Texture;

layout (push_constant) uniform Uniforms
{
	float TextureLod;
} u_Uniforms;

layout (location = 0) in vec3 v_Position;

void main()
{
	finalColor = textureLod(u_Texture, v_Position, u_Uniforms.TextureLod);
}
