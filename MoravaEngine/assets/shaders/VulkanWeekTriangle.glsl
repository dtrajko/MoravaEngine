// Vulkan Triangle shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

layout (location = 0) out vec4 v_Color;

void main()
{
	gl_Position = vec4(a_Position, 1.0);
	v_Color = a_Color;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 finalColor;

layout (location = 0) in vec4 v_Color;

void main()
{
	finalColor = v_Color;
}
