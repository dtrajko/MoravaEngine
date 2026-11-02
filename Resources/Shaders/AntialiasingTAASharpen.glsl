// TAA, last pass (Vulkan, SceneEnvMapVulkan, 8.2 of docs/engine/Game_Engine_Runtime_Guide.html): the resolve's result, a
// little sharper, because averaging over the frames softens it. Limited to the range of the pixel's direct neighbors, so
// that edges get no bright or dark halos. Or, for the Environment panel's debug view, how far the history was pulled.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

void main()
{
	gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_ColorTexture; // the resolve's result; alpha: how far the history was pulled

layout(push_constant) uniform Uniforms
{
	float View; // 0: the picture, 3: the TAA Rejection debug view (EnvMapVulkanAntialiasing::Output)
} u_Uniforms;

const float Strength = 0.25;

void main()
{
	ivec2 size = textureSize(u_ColorTexture, 0);
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec4 center = texelFetch(u_ColorTexture, pixel, 0);
	if (u_Uniforms.View > 0.5)
	{
		// Black: the history was kept. Toward white: it was rejected. Over a dim copy of the picture, to see what is where.
		o_Color = vec4(mix(center.rgb * 0.25, vec3(1.0, 0.9, 0.2), center.a), 1.0);
		return;
	}

	vec3 left = texelFetch(u_ColorTexture, clamp(pixel + ivec2(-1, 0), ivec2(0), size - 1), 0).rgb;
	vec3 right = texelFetch(u_ColorTexture, clamp(pixel + ivec2(1, 0), ivec2(0), size - 1), 0).rgb;
	vec3 up = texelFetch(u_ColorTexture, clamp(pixel + ivec2(0, -1), ivec2(0), size - 1), 0).rgb;
	vec3 down = texelFetch(u_ColorTexture, clamp(pixel + ivec2(0, 1), ivec2(0), size - 1), 0).rgb;
	vec3 lowest = min(center.rgb, min(min(left, right), min(up, down)));
	vec3 highest = max(center.rgb, max(max(left, right), max(up, down)));
	vec3 sharpened = center.rgb + (center.rgb - (left + right + up + down) * 0.25) * Strength;
	o_Color = vec4(clamp(sharpened, lowest, highest), 1.0);
}
