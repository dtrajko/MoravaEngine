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
// The G-buffer (see EnvMapVulkanGBuffer), for the debug views
layout (binding = 5) uniform sampler2D u_GBufferNormalRoughness; // xyz = world-space normal (0: no surface, the sky), w = roughness
layout (binding = 6) uniform sampler2D u_GBufferMotion;          // screen-space motion in UV units
layout (binding = 7) uniform sampler2D u_GBufferDepth;           // the scene's depth

layout(push_constant) uniform Uniforms
{
	float Exposure;
	float BloomIntensity;     // 0 when bloom is disabled
	float BloomDirtIntensity;
	float OutlineWidth;       // pixels; 0 = no outline
	vec4 OutlineColor;
	float HuePreservation;    // 0: ACES per channel (bright colors turn white), 1: hue-preserving ACES (see Tonemap)
	float RawScene;           // 1: the scene as it is, without exposure, bloom and tonemapping (Shadows Only view)
	float DebugView;          // a G-buffer view instead of the scene: 1 normals, 2 roughness, 3 linear depth, 4 motion (0: off)
	vec4 DebugParams;         // xy = the projection matrix's [2][2] and [3][2] (depth -> distance), z = the distance shown as
	                          // white (linear depth), w = the motion shown at full color, in pixels
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

// The G-buffer as a display color (shown as it is: no exposure, tonemapping or gamma, so the values can be read off the
// screen). Where there is no surface (the sky): black, white in the depth view.
vec3 GBufferView(int view)
{
	vec4 normalRoughness = texture(u_GBufferNormalRoughness, Input.TexCoord);
	bool surface = dot(normalRoughness.xyz, normalRoughness.xyz) > 0.0;
	if (view == 1)
	{
		// The world-space normal: x red, y green, z blue, from -1..1 to 0..1 (straight up is light green)
		return surface ? normalize(normalRoughness.xyz) * 0.5 + 0.5 : vec3(0.0);
	}
	if (view == 2)
	{
		return surface ? vec3(normalRoughness.w) : vec3(0.0);
	}
	if (view == 3)
	{
		// The distance along the camera's view direction, from the depth and the projection's two depth terms
		float depth = texture(u_GBufferDepth, Input.TexCoord).r;
		float distance = u_Uniforms.DebugParams.y / (depth + u_Uniforms.DebugParams.x);
		return depth < 1.0 ? vec3(clamp(distance / max(u_Uniforms.DebugParams.z, 0.001), 0.0, 1.0)) : vec3(1.0);
	}
	// Motion since the previous frame, in pixels: mid gray = none, more red = to the right, more green = down
	vec2 pixels = texture(u_GBufferMotion, Input.TexCoord).rg * vec2(textureSize(u_GBufferMotion, 0));
	return vec3(clamp(0.5 + pixels * 0.5 / max(u_Uniforms.DebugParams.w, 0.001), 0.0, 1.0), 0.5);
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
	if (u_Uniforms.DebugView > 0.5)
	{
		displayColor = GBufferView(int(u_Uniforms.DebugView + 0.5));
	}

	vec4 overlay = texture(u_OverlayTexture, Input.TexCoord);
	displayColor = mix(displayColor, overlay.rgb, overlay.a);
	displayColor = mix(displayColor, u_Uniforms.OutlineColor.rgb, SelectionOutline() * u_Uniforms.OutlineColor.a);

	o_Color = vec4(displayColor, 1.0);
}
