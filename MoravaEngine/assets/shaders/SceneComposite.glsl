#type vertex
#version 430

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main()
{
	vec4 position = vec4(a_Position.xy, 0.0, 1.0);
	v_TexCoord = a_TexCoord;
	gl_Position = position;
}

#type fragment
#version 430

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_BloomTexture;

in vec2 v_TexCoord;

uniform sampler2DMS u_Texture;

uniform float u_Exposure;
uniform int u_TextureSamples;

uniform bool u_EnableBloom;
uniform float u_BloomThreshold;

const float uFar = 1.0;

vec4 SampleTexture(sampler2D tex, vec2 texCoord)
{
    return texture(tex, texCoord);
}

vec4 MultiSampleTexture(sampler2DMS tex, vec2 tc)
{
	ivec2 texSize = textureSize(tex);
	ivec2 texCoord = ivec2(tc * texSize);
	vec4 result = vec4(0.0);
    for (int i = 0; i < u_TextureSamples; i++)
        result += texelFetch(tex, texCoord, i);

    result /= float(u_TextureSamples);
    return result;
}

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

	// Tonemapping
	vec4 msColor = MultiSampleTexture(u_Texture, v_TexCoord);

	vec3 color = msColor.rgb;

	if (u_EnableBloom)
	{
		vec3 bloomColor = MultiSampleTexture(u_Texture, v_TexCoord).rgb;
		color += bloomColor;
	}

	color *= u_Exposure;

	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	o_Color = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);

	// Show over-exposed areas
	// if (o_Color.r > 1.0 || o_Color.g > 1.0 || o_Color.b > 1.0)
	// 	o_Color.rgb *= vec3(1.0, 0.25, 0.25);
}