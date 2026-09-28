#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

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

layout(push_constant) uniform Uniforms
{
	float Exposure;
} u_Uniforms;

void main()
{
	const float gamma     = 2.2;

	vec3 color = texture(u_Texture, Input.TexCoord).rgb * u_Uniforms.Exposure;

	// ACES filmic tonemapping (Narkowicz fit, with its 0.6 pre-exposure). The previous Reinhard version used
	// pureWhite = 1.0, which reduces to the identity: HDR values above 1.0 were simply clipped.
	vec3 x = max(color, vec3(0.0)) * 0.6;
	vec3 mappedColor = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);

	// Gamma correction.
	o_Color = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);

	// Show over-exposed areas
	// if (o_Color.r > 1.0 || o_Color.g > 1.0 || o_Color.b > 1.0)
	// 	o_Color.rgb *= vec3(1.0, 0.25, 0.25);
}
