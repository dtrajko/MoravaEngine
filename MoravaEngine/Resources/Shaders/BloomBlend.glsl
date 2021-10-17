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

in vec2 v_TexCoord;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_BloomTexture;

uniform float u_Exposure;
uniform bool u_EnableBloom;

void main()
{
#if 1
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

    // Tonemapping
	vec3 color = texture(u_SceneTexture, v_TexCoord).rgb;
	if (u_EnableBloom)
	{
		vec3 bloomColor = texture(u_BloomTexture, v_TexCoord).rgb;
		color += bloomColor;
	}
	
	// Reinhard tonemapping
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color* u_Exposure;

	// Gamma correction.
	o_Color = vec4(color, 1.0);
#else
	const float gamma = 2.2;
    vec3 hdrColor = texture(u_SceneTexture, v_TexCoord).rgb;      
    vec3 bloomColor = texture(u_BloomTexture, v_TexCoord).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * u_Exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    o_Color = vec4(result, 1.0);
#endif
}