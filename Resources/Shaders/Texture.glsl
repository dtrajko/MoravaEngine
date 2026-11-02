#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;

// layout (location = 0) in vec3 a_Position;
// layout (location = 1) in vec3 a_Normal;
// layout (location = 2) in vec3 a_Tangent;
// layout (location = 3) in vec3 a_Binormal;
// layout (location = 4) in vec2 a_TexCoord;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) out OutputBlock Output;

void main()
{
	vec4 position = vec4(a_Position.xy, 0.0, 1.0);
	Output.TexCoord = a_TexCoord;
	gl_Position = position;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;

layout (binding = 0) uniform sampler2D u_Texture;

void main()
{
	o_Color = texture(u_Texture, Input.TexCoord);
}