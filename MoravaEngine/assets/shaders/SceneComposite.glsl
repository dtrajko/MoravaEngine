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

layout (location = 0, binding = 0) uniform sampler2DMS u_Texture;

layout(push_constant) uniform Uniforms
{
	float Exposure;
	int TextureSamples;
} u_Uniforms;

vec4 MultiSampleTexture(sampler2DMS tex, ivec2 texCoord, int samples)
{
    vec4 result = vec4(0.0);
    for (int i = 0; i < samples; i++)
        result += texelFetch(tex, texCoord, i);

    result /= float(samples);
    return result;
}

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

	ivec2 texSize = textureSize(u_Texture);
	ivec2 texCoord = ivec2(Input.TexCoord * texSize);
	vec4 msColor = MultiSampleTexture(u_Texture, texCoord, u_Uniforms.TextureSamples);

	vec3 color = msColor.rgb * u_Uniforms.Exposure;//texture(u_Texture, v_TexCoord).rgb * u_Exposure;
	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	o_Color = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);
}
