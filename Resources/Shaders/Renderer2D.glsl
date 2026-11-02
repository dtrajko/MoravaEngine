// Basic Texture Shader

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;

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
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) out VertexOutput Output;
layout (location = 5) out flat float TexIndex;

void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	TexIndex = a_TexIndex;
	Output.TilingFactor = a_TilingFactor;
	gl_Position = u_ViewProjection * u_Renderer.Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) in VertexOutput Input;
layout (location = 5) in flat float TexIndex;

// 32 textures without samplers of their own, read with one shared sampler (VulkanShaderH2M::GetSharedSampler): Metal
// (macOS) allows 16 samplers per shader stage, and an array of sampler2D counts each element as one
layout (binding = 1) uniform texture2D u_Textures[32];
layout (binding = 2) uniform sampler s_LinearRepeat;

void main()
{
	color = texture(sampler2D(u_Textures[int(TexIndex)], s_LinearRepeat), Input.TexCoord * Input.TilingFactor) * Input.Color;
	if (color.a < 0.5)
		discard;
}