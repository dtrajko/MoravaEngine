// Shadow Map shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
	mat4 u_InverseViewProjectionMatrix;
	mat4 u_ProjectionMatrix;
	mat4 u_ViewMatrix;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

layout(location = 0) out float LinearDepth;

void main()
{
	vec4 worldPosition = u_Renderer.Transform * vec4(a_Position, 1.0);

	LinearDepth = -(u_ViewMatrix * worldPosition).z;

	gl_Position = u_ViewProjectionMatrix * worldPosition;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_LinearDepth;

layout(location = 0) in float LinearDepth;

void main()
{
	// TODO: Check for alpha in texture
	o_LinearDepth = vec4(LinearDepth, 0.0, 0.0, 1.0);
}