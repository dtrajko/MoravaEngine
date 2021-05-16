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

void main()
{
	gl_Position = vec4(a_Position, 1.0);
	v_TexCoord = a_TexCoord;
	v_Normal = a_Normal;
}

#type fragment
#version 450 core

layout (location = 0) in vec3 v_Normal;
layout (location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 finalColor;

void main()
{
	finalColor = vec4(v_Normal, 1.0);
}
