// Basic Texture Shader

#type vertex
#version 430 core

layout(location = 0) in vec3 a_WorldPosition;
layout(location = 1) in float a_Thickness;
layout(location = 2) in vec2 a_LocalPosition;
layout(location = 3) in vec4 a_Color;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} u_Renderer;

struct VertexOutput
{
	vec2 LocalPosition;
	float Thickness;
	vec4 Color;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.LocalPosition = a_LocalPosition;
	Output.Thickness = a_Thickness;
	Output.Color = a_Color;
	gl_Position = u_ViewProjection * u_Renderer.Transform * vec4(a_WorldPosition, 1.0);
}

#type fragment
#version 430 core

layout(location = 0) out vec4 color;

struct VertexOutput
{
	vec2 LocalPosition;
	float Thickness;
	vec4 Color;
};

layout (location = 0) in VertexOutput Input;

void main()
{
	float fade = 0.01;
	float dist = sqrt(dot(Input.LocalPosition, Input.LocalPosition));
	if (dist > 1.0 || dist < 1.0 - Input.Thickness - fade)
		discard;

	float alpha = 1.0 - smoothstep(1.0f - fade, 1.0f, dist);
	alpha *= smoothstep(1.0 - Input.Thickness - fade, 1.0 - Input.Thickness, dist);
	color = Input.Color;
	color.a = alpha;
}