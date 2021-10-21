// Outline Shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

//////////////////////////////////////////
// UNUSED
//////////////////////////////////////////
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
//////////////////////////////////////////

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

void main()
{
	gl_Position = u_ViewProjection * u_Renderer.Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

layout (push_constant) uniform Material
{
	layout (offset = 64) vec4 Color;
} u_MaterialUniforms;

void main()
{
	color = u_MaterialUniforms.Color;
}
