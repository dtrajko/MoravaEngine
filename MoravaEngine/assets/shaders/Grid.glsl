// Grid Shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 ViewProjection;
};

layout (push_constant) uniform VertexUniforms
{
	mat4 Transform;
} u_VertexUniforms;

layout (location = 0) out vec2 v_TexCoord;

void main()
{
	vec4 position = ViewProjection * u_VertexUniforms.Transform * vec4(a_Position, 1.0);
	gl_Position = position;

	v_TexCoord = a_TexCoord;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

layout (push_constant) uniform Settings
{
	float Scale;
	float Size;
} u_Settings;

layout (location = 0) in vec2 v_TexCoord;

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}

void main()
{
	float x = grid(v_TexCoord * u_Settings.Scale, u_Settings.Size);
	color = vec4(vec3(0.2), 0.5) * (1.0 - x);
}
