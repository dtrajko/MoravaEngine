// Basic Texture Shader

#type vertex
#version 430 core

layout(location = 0) in vec3 a_WorldPosition;
layout(location = 1) in float a_Thickness;
layout(location = 2) in vec2 a_LocalPosition;
layout(location = 3) in vec4 a_Color;
uniform mat4 u_ViewProjection;

out vec2 v_LocalPosition;
out float v_Thickness;
out vec4 v_Color;

void main()
{
	v_LocalPosition = a_LocalPosition;
	v_Thickness = a_Thickness;
	v_Color = a_Color;
	gl_Position = u_ViewProjection * vec4(a_WorldPosition, 1.0);
}

#type fragment
#version 430 core

layout(location = 0) out vec4 color;

in vec2 v_LocalPosition;
in float v_Thickness;
in vec4 v_Color;

void main()
{
	float fade = 0.01;
	float dist = sqrt(dot(v_LocalPosition, v_LocalPosition));
	if (dist > 1.0 || dist < 1.0 - v_Thickness - fade)
		discard;

	float alpha = 1.0 - smoothstep(1.0f - fade, 1.0f, dist);
	alpha *= smoothstep(1.0 - v_Thickness - fade, 1.0 - v_Thickness, dist);
	color = v_Color;
	color.a = alpha;
}