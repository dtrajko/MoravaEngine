// TAA, resolve (Vulkan, SceneEnvMapVulkan, 8.2 of docs/engine/Game_Engine_Runtime_Guide.html): this frame's picture, whose
// camera was moved by less than a pixel (the jitter), mixed with the history: the result of the frames before, read where
// each surface point was in the previous frame. Over the frames every pixel gets the average of many sample positions.
//
// The history can be wrong (what the pixel shows now was hidden, its light changed, glass or water moved with no motion
// vector of their own): it is pulled into the range of this frame's colors around the pixel (variance clipping).
//
// On the composite: gamma encoded and tonemapped, so a bright highlight can't outweigh its neighbors. The output's alpha
// holds how far the history was pulled, for the TAA Rejection debug view.
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

layout (binding = 0) uniform sampler2D u_ColorTexture;   // the composite
layout (binding = 1) uniform sampler2D u_HistoryTexture; // the previous frame's result
layout (binding = 2) uniform sampler2D u_MotionTexture;  // the G-buffer's: this frame's position minus the previous one's, in UV
layout (binding = 3) uniform sampler2D u_DepthTexture;   // the scene's depth, 1: no surface (the sky)

// Must match EnvMapVulkanAntialiasing::ResolvePush
layout(push_constant) uniform Uniforms
{
	mat4 CurrentToPrevious; // from this frame's clip space to the previous frame's, the camera alone and without the jitter
	vec2 JitterMotion;      // what the jitter adds to the G-buffer's motion: the two frames' offsets differ
	float HasHistory;       // 0: the first frame, after a resize or a new scene
	float Padding;
} u_Uniforms;

const float NewFrameWeight = 0.1;       // of a pixel at rest; the rest comes from the history
const float MovingNewFrameWeight = 0.2; // of a pixel moving fast, whose history is resampled and so softer
const float MovingPixels = 8.0;         // how many pixels per frame count as fast
const float ClipRange = 1.25;           // standard deviations of the neighborhood the history may be from its mean

vec3 ToYCoCg(vec3 c)
{
	return vec3(dot(c, vec3(0.25, 0.5, 0.25)), dot(c, vec3(0.5, 0.0, -0.5)), dot(c, vec3(-0.25, 0.5, -0.25)));
}

vec3 FromYCoCg(vec3 c)
{
	return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

// Catmull-Rom in five bilinear reads (the corners matter little): resampling the history every frame stays sharp
vec3 SampleHistory(vec2 uv)
{
	vec2 size = vec2(textureSize(u_HistoryTexture, 0));
	vec2 position = uv * size;
	vec2 center = floor(position - 0.5) + 0.5;
	vec2 f = position - center;
	vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
	vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
	vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
	vec2 w3 = f * f * (-0.5 + 0.5 * f);
	vec2 w12 = w1 + w2;
	vec2 tc0 = (center - 1.0) / size;
	vec2 tc12 = (center + w2 / w12) / size;
	vec2 tc3 = (center + 2.0) / size;

	vec3 sum = textureLod(u_HistoryTexture, vec2(tc12.x, tc0.y), 0.0).rgb * (w12.x * w0.y);
	sum += textureLod(u_HistoryTexture, vec2(tc0.x, tc12.y), 0.0).rgb * (w0.x * w12.y);
	sum += textureLod(u_HistoryTexture, tc12, 0.0).rgb * (w12.x * w12.y);
	sum += textureLod(u_HistoryTexture, vec2(tc3.x, tc12.y), 0.0).rgb * (w3.x * w12.y);
	sum += textureLod(u_HistoryTexture, vec2(tc12.x, tc3.y), 0.0).rgb * (w12.x * w3.y);
	float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
	return max(sum / weight, vec3(0.0)); // its negative lobes can go below black
}

void main()
{
	ivec2 size = textureSize(u_ColorTexture, 0);
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	vec3 current = texelFetch(u_ColorTexture, pixel, 0).rgb;

	// The 3x3 neighborhood: its colors' mean and spread, and its nearest surface, whose motion this pixel takes (else an
	// edge pixel showing the background would lose the moving edge's history and stay jagged)
	vec3 mean = vec3(0.0), squares = vec3(0.0);
	float nearest = 1.0;
	ivec2 nearestPixel = pixel;
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			ivec2 neighbor = clamp(pixel + ivec2(x, y), ivec2(0), size - 1);
			vec3 color = ToYCoCg(texelFetch(u_ColorTexture, neighbor, 0).rgb);
			mean += color;
			squares += color * color;
			float depth = texelFetch(u_DepthTexture, neighbor, 0).r;
			if (depth < nearest)
			{
				nearest = depth;
				nearestPixel = neighbor;
			}
		}
	}
	mean /= 9.0;
	vec3 deviation = sqrt(max(squares / 9.0 - mean * mean, vec3(0.0)));

	// The motion: the sky isn't in the G-buffer, so it moves with the camera alone
	vec2 motion;
	if (nearest >= 1.0)
	{
		vec2 ndc = v_TexCoord * 2.0 - 1.0;
		vec4 previous = u_Uniforms.CurrentToPrevious * vec4(ndc, 1.0, 1.0);
		motion = (ndc - previous.xy / previous.w) * 0.5;
	}
	else
	{
		motion = texelFetch(u_MotionTexture, nearestPixel, 0).rg - u_Uniforms.JitterMotion;
	}

	vec2 previousUV = v_TexCoord - motion;
	if (u_Uniforms.HasHistory < 0.5 || any(lessThan(previousUV, vec2(0.0))) || any(greaterThan(previousUV, vec2(1.0))))
	{
		o_Color = vec4(current, 1.0); // nothing to mix with: the pixel starts over
		return;
	}

	// The history pulled toward the neighborhood's mean until it is inside its range
	vec3 history = ToYCoCg(SampleHistory(previousUV));
	vec3 fromMean = history - mean;
	vec3 reach = max(deviation * ClipRange, vec3(0.0001));
	float outside = max(max(abs(fromMean.x) / reach.x, abs(fromMean.y) / reach.y), abs(fromMean.z) / reach.z);
	vec3 clipped = outside > 1.0 ? mean + fromMean / outside : history;

	float moved = length(motion * vec2(size));
	float newFrame = mix(NewFrameWeight, MovingNewFrameWeight, clamp(moved / MovingPixels, 0.0, 1.0));
	vec3 result = mix(FromYCoCg(clipped), current, newFrame);
	o_Color = vec4(clamp(result, 0.0, 1.0), clamp(length(history - clipped) * 4.0, 0.0, 1.0));
}
