// Viewport composite (Vulkan, SceneEnvMapVulkan): the linear HDR scene plus bloom, then exposure, ACES tonemapping
// and gamma. Same as SceneComposite.glsl (still used by SceneHazelVulkan), with the bloom inputs added.
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
	Output.TexCoord = a_TexCoord;
	gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;

layout (binding = 0) uniform sampler2D u_Texture;           // linear HDR scene
layout (binding = 1) uniform sampler2D u_BloomTexture;      // bloom (already exposed, see BloomPass.glsl)
layout (binding = 2) uniform sampler2D u_BloomDirtTexture;  // lens dirt, lit by the bloom

layout(push_constant) uniform Uniforms
{
	float Exposure;
	float BloomIntensity;     // 0 when bloom is disabled
	float BloomDirtIntensity;
} u_Uniforms;

void main()
{
	const float gamma = 2.2;

	vec3 color = texture(u_Texture, Input.TexCoord).rgb * u_Uniforms.Exposure;

	vec3 bloom = texture(u_BloomTexture, Input.TexCoord).rgb;
	vec3 dirt = texture(u_BloomDirtTexture, Input.TexCoord).rgb;
	color += bloom * u_Uniforms.BloomIntensity;
	color += bloom * dirt * u_Uniforms.BloomDirtIntensity;

	// ACES filmic tonemapping (Narkowicz fit, with its 0.6 pre-exposure)
	vec3 x = max(color, vec3(0.0)) * 0.6;
	vec3 mappedColor = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);

	o_Color = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);
}
