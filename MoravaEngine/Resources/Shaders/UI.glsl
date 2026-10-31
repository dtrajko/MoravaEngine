// UI (Vulkan, SceneEnvMapVulkan): the game UI's quads (docs/engine/Game_Engine_Runtime_Guide.html, 7.2), drawn into the
// viewport composite after tonemapping, so their colors reach the screen as given. Images: the texture times the color.
// Text: the texture is a multi-channel signed distance field font atlas (UI/UIFont); the median of its three channels is
// the distance to the glyph's outline, 0.5 on it, and DistancePixels turns the distance into screen pixels for an edge
// about one pixel soft at any size. An image of an sRGB texture (a Color render texture, 7.5) is decoded to linear
// light when sampled: it is encoded again, so its texels reach the screen as they are; an image of a float texture (an
// HDR render texture) is light, exposed and tonemapped as the scene is.
#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;     // view pixels, from the top-left corner
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_Mode;        // 0: image, 1: text, 2: image of an sRGB texture, 3: image of an HDR texture
layout(location = 4) in float a_DistancePixels;

layout (push_constant) uniform View
{
	vec2 Size;                               // the view in pixels
} u_View;

struct OutputBlock
{
	vec4 Color;
	vec2 TexCoord;
	float Mode;
	float DistancePixels;
};

layout (location = 0) out OutputBlock Output;

void main()
{
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	Output.Mode = a_Mode;
	Output.DistancePixels = a_DistancePixels;
	// The composite image is shown with its first row at the bottom: y = 0 (the top) goes to +1
	vec2 ndc = a_Position / u_View.Size * 2.0 - 1.0;
	gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

struct OutputBlock
{
	vec4 Color;
	vec2 TexCoord;
	float Mode;
	float DistancePixels;
};

layout (location = 0) in OutputBlock Input;

layout (binding = 0) uniform sampler2D u_Texture; // an image, a font atlas or a white texel

// The scene's exposure and tonemapping, for HDR images (after the vertex stage's View)
layout (push_constant) uniform Tone
{
	layout (offset = 16) float Exposure;
	float HuePreservation;
} u_Tone;

#include "Include/Tonemap.glslh"

// Linear light back to sRGB's encoding (the exact curve the GPU decodes with)
vec3 EncodeSRGB(vec3 linear)
{
	linear = clamp(linear, 0.0, 1.0);
	return mix(linear * 12.92, 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), linear));
}

float Median(vec3 v)
{
	return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main()
{
	if (Input.Mode > 0.5 && Input.Mode < 1.5)
	{
		// The atlas has mips (all textures do), but a distance field must be read at full resolution
		vec3 distances = textureLod(u_Texture, Input.TexCoord, 0.0).rgb;
		float screenDistance = Input.DistancePixels * (Median(distances) - 0.5);
		float coverage = clamp(screenDistance + 0.5, 0.0, 1.0);
		if (coverage <= 0.0)
		{
			discard;
		}
		o_Color = vec4(Input.Color.rgb, Input.Color.a * coverage);
	}
	else
	{
		vec4 texel = texture(u_Texture, Input.TexCoord);
		if (Input.Mode > 2.5)
		{
			texel.rgb = pow(Tonemap(texel.rgb * u_Tone.Exposure, u_Tone.HuePreservation), vec3(1.0 / 2.2)); // as ViewportComposite.glsl
		}
		else if (Input.Mode > 1.5)
		{
			texel.rgb = EncodeSRGB(texel.rgb);
		}
		o_Color = Input.Color * texel;
	}
}
