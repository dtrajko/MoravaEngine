// FXAA (Vulkan, SceneEnvMapVulkan, 8.1 of docs/engine/Game_Engine_Runtime_Guide.html): the cheaper one-pass antialiasing,
// after Timothy Lottes's FXAA: a pixel on a contrast edge moves its sample across the edge, by how far it is from the
// edge's nearer end, and by the local contrast for details smaller than a pixel. Softer than SMAA.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (location = 0) out vec2 v_TexCoord;

void main()
{
	// From the position, not a_TexCoord: pixel for pixel, each pass leaves the picture as it is
	v_TexCoord = a_Position.xy * 0.5 + 0.5;
	gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout (location = 0) in vec2 v_TexCoord;

layout (binding = 0) uniform sampler2D u_ColorTexture; // the composite: gamma encoded

const float ContrastThreshold = 0.0312;
const float RelativeThreshold = 0.125;
const float SubpixelStrength = 0.75;
const int SearchSteps = 10;
const float SearchStepSizes[SearchSteps] = float[](1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 4.0, 8.0);

float Luma(vec3 color)
{
	return dot(color, vec3(0.299, 0.587, 0.114));
}

float LumaAt(vec2 uv)
{
	return Luma(textureLod(u_ColorTexture, uv, 0.0).rgb);
}

void main()
{
	vec2 texel = 1.0 / vec2(textureSize(u_ColorTexture, 0));
	vec3 center = textureLod(u_ColorTexture, v_TexCoord, 0.0).rgb;
	float m = Luma(center);
	float n = LumaAt(v_TexCoord + vec2(0.0, -texel.y));
	float s = LumaAt(v_TexCoord + vec2(0.0, texel.y));
	float e = LumaAt(v_TexCoord + vec2(texel.x, 0.0));
	float w = LumaAt(v_TexCoord + vec2(-texel.x, 0.0));

	float highest = max(m, max(max(n, s), max(e, w)));
	float lowest = min(m, min(min(n, s), min(e, w)));
	float contrast = highest - lowest;
	if (contrast < max(ContrastThreshold, highest * RelativeThreshold))
	{
		o_Color = vec4(center, 1.0);
		return;
	}

	float nw = LumaAt(v_TexCoord + vec2(-texel.x, -texel.y));
	float ne = LumaAt(v_TexCoord + vec2(texel.x, -texel.y));
	float sw = LumaAt(v_TexCoord + vec2(-texel.x, texel.y));
	float se = LumaAt(v_TexCoord + vec2(texel.x, texel.y));

	// Details smaller than a pixel: how much the pixel differs from its neighbors' average
	float average = (2.0 * (n + s + e + w) + nw + ne + sw + se) / 12.0;
	float subpixel = smoothstep(0.0, 1.0, clamp(abs(average - m) / contrast, 0.0, 1.0));
	subpixel = subpixel * subpixel * SubpixelStrength;

	// The edge's direction, and which side of it this pixel looks across
	float horizontalChange = abs(n + s - 2.0 * m) * 2.0 + abs(ne + se - 2.0 * e) + abs(nw + sw - 2.0 * w);
	float verticalChange = abs(e + w - 2.0 * m) * 2.0 + abs(ne + nw - 2.0 * n) + abs(se + sw - 2.0 * s);
	bool horizontal = horizontalChange >= verticalChange;

	float positiveLuma = horizontal ? s : e;
	float negativeLuma = horizontal ? n : w;
	float positiveGradient = abs(positiveLuma - m);
	float negativeGradient = abs(negativeLuma - m);
	bool positive = positiveGradient >= negativeGradient;
	float stepAcross = (horizontal ? texel.y : texel.x) * (positive ? 1.0 : -1.0);
	float otherLuma = positive ? positiveLuma : negativeLuma;
	float gradient = max(positiveGradient, negativeGradient);

	// Along the edge, both ways, to its ends
	vec2 uvEdge = v_TexCoord;
	vec2 along;
	if (horizontal)
	{
		uvEdge.y += stepAcross * 0.5;
		along = vec2(texel.x, 0.0);
	}
	else
	{
		uvEdge.x += stepAcross * 0.5;
		along = vec2(0.0, texel.y);
	}
	float edgeLuma = (m + otherLuma) * 0.5;
	float threshold = gradient * 0.25;

	vec2 uvPositive = uvEdge;
	vec2 uvNegative = uvEdge;
	float deltaPositive = 0.0;
	float deltaNegative = 0.0;
	bool atPositiveEnd = false;
	bool atNegativeEnd = false;
	for (int i = 0; i < SearchSteps && !(atPositiveEnd && atNegativeEnd); i++)
	{
		if (!atPositiveEnd)
		{
			uvPositive += along * SearchStepSizes[i];
			deltaPositive = LumaAt(uvPositive) - edgeLuma;
			atPositiveEnd = abs(deltaPositive) >= threshold;
		}
		if (!atNegativeEnd)
		{
			uvNegative -= along * SearchStepSizes[i];
			deltaNegative = LumaAt(uvNegative) - edgeLuma;
			atNegativeEnd = abs(deltaNegative) >= threshold;
		}
	}

	float distancePositive = horizontal ? uvPositive.x - v_TexCoord.x : uvPositive.y - v_TexCoord.y;
	float distanceNegative = horizontal ? v_TexCoord.x - uvNegative.x : v_TexCoord.y - uvNegative.y;
	float nearest = min(distancePositive, distanceNegative);
	float nearEndDelta = distancePositive <= distanceNegative ? deltaPositive : deltaNegative;

	// Only where the nearer end's luma goes the other way than this pixel's: the pixel is on the edge's stair step
	float edgeOffset = 0.0;
	if ((nearEndDelta < 0.0) != (m - edgeLuma < 0.0))
	{
		edgeOffset = 0.5 - nearest / (distancePositive + distanceNegative);
	}

	float offset = max(edgeOffset, subpixel);
	vec2 uv = v_TexCoord;
	if (horizontal)
	{
		uv.y += offset * stepAcross;
	}
	else
	{
		uv.x += offset * stepAcross;
	}
	o_Color = vec4(textureLod(u_ColorTexture, uv, 0.0).rgb, 1.0);
}
