// Viewport composite (Vulkan, SceneEnvMapVulkan): the linear HDR scene plus bloom, then exposure, ACES tonemapping (optionally hue-preserving)
// and gamma. Same as SceneComposite.glsl (still used by SceneHazelVulkan), with the bloom inputs added.
// The editor overlays (wireframe, bounding boxes) and the selection outline are added after tonemapping, so they keep
// their exact colors: exposure, tonemapping and bloom don't apply to them.
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
layout (binding = 3) uniform sampler2D u_OverlayTexture;    // wireframe and bounding boxes (display colors, alpha = coverage)
layout (binding = 4) uniform sampler2D u_SelectionMask;     // silhouette of the selected mesh/submesh (r = 1)

layout(push_constant) uniform Uniforms
{
	float Exposure;
	float BloomIntensity;     // 0 when bloom is disabled
	float BloomDirtIntensity;
	float OutlineWidth;       // pixels; 0 = no outline
	vec4 OutlineColor;
	float HuePreservation;    // 0: ACES per channel (bright colors turn white), 1: hue-preserving ACES (see Tonemap)
	float RawScene;           // 1: the scene as it is, without exposure, bloom and tonemapping (Shadows Only view)
} u_Uniforms;

// ACES filmic tonemapping (Narkowicz fit, with its 0.6 pre-exposure), per channel
vec3 ACES(vec3 color)
{
	vec3 x = max(color, vec3(0.0)) * 0.6;
	return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

// Per channel, the brightest channel of a bright color reaches 1 first while the others keep rising, so a bright red
// light turns white at its center (as film does). The hue-preserving version tonemaps only the brightest channel and
// scales the whole color by the same factor: the ratios between the channels (hue and saturation) stay as lit.
vec3 Tonemap(vec3 color)
{
	color = max(color, vec3(0.0));
	vec3 perChannel = ACES(color);
	float peak = max(color.r, max(color.g, color.b));
	vec3 huePreserving = peak > 0.0 ? color * (ACES(vec3(peak)).r / peak) : vec3(0.0);
	return mix(perChannel, huePreserving, clamp(u_Uniforms.HuePreservation, 0.0, 1.0));
}

// 1 on pixels just outside the selection's silhouette (within OutlineWidth pixels of it), 0 elsewhere
float SelectionOutline()
{
	int radius = int(ceil(u_Uniforms.OutlineWidth));
	if (radius <= 0)
	{
		return 0.0;
	}

	ivec2 size = textureSize(u_SelectionMask, 0);
	ivec2 pixel = ivec2(Input.TexCoord * vec2(size));
	if (texelFetch(u_SelectionMask, clamp(pixel, ivec2(0), size - 1), 0).r > 0.5)
	{
		return 0.0; // inside: the selected mesh stays visible
	}

	float coverage = 0.0;
	for (int y = -radius; y <= radius; y++)
	{
		for (int x = -radius; x <= radius; x++)
		{
			ivec2 neighbor = clamp(pixel + ivec2(x, y), ivec2(0), size - 1);
			if (texelFetch(u_SelectionMask, neighbor, 0).r > 0.5)
			{
				// Round outline with a soft (anti-aliased) edge
				coverage = max(coverage, clamp(u_Uniforms.OutlineWidth + 0.5 - length(vec2(x, y)), 0.0, 1.0));
			}
		}
	}
	return coverage;
}

void main()
{
	const float gamma = 2.2;

	vec3 color = texture(u_Texture, Input.TexCoord).rgb * u_Uniforms.Exposure;

	vec3 bloom = texture(u_BloomTexture, Input.TexCoord).rgb;
	vec3 dirt = texture(u_BloomDirtTexture, Input.TexCoord).rgb;
	color += bloom * u_Uniforms.BloomIntensity;
	color += bloom * dirt * u_Uniforms.BloomDirtIntensity;

	vec3 mappedColor = Tonemap(color);
	if (u_Uniforms.RawScene > 0.5)
	{
		mappedColor = clamp(texture(u_Texture, Input.TexCoord).rgb, 0.0, 1.0);
	}

	vec3 displayColor = pow(mappedColor, vec3(1.0 / gamma));

	vec4 overlay = texture(u_OverlayTexture, Input.TexCoord);
	displayColor = mix(displayColor, overlay.rgb, overlay.a);
	displayColor = mix(displayColor, u_Uniforms.OutlineColor.rgb, SelectionOutline() * u_Uniforms.OutlineColor.a);

	o_Color = vec4(displayColor, 1.0);
}
