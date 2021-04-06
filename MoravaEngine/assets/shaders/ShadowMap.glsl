// Shadow Map shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout (std140, binding = 1) uniform ShadowData
{
	mat4 u_ViewProjectionMatrix;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

void main()
{
	gl_Position = u_ViewProjectionMatrix * u_Renderer.Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

void main()
{
	// TODO: Check for alpha in texture
}