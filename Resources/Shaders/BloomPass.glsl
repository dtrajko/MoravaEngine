// Bloom (Vulkan, SceneEnvMapVulkan): fragment shader version of the downsample/upsample chain used by Hazel's compute
// bloom (Resources/Shaders/PostProcessing/Bloom.glsl), after "Next Generation Post Processing in Call of Duty: Advanced
// Warfare" (Jorge Jimenez, SIGGRAPH 2014). Every pass draws a fullscreen quad into one level of the bloom chain:
//   Prefilter:  scene (full resolution) -> level 0 (half resolution), keeps only the bright parts (threshold, knee)
//   Downsample: level i-1 -> level i (half of it), 13-tap filter
//   Upsample:   level i + tent-filtered (level i+1 upsampled so far) -> upsample level i
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

layout (binding = 0) uniform sampler2D u_Texture;      // prefilter/downsample: the level above; upsample: the same level
layout (binding = 1) uniform sampler2D u_BloomTexture; // upsample: the smaller level (upsampled so far)

layout(push_constant) uniform Uniforms
{
	vec4 Params;         // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
	float UpsampleScale; // tent filter radius, in texels of the smaller level
	float Exposure;      // the threshold applies to the exposed scene, as seen in the viewport
	int Mode;
} u_Uniforms;

#define MODE_PREFILTER  0
#define MODE_DOWNSAMPLE 1
#define MODE_UPSAMPLE   2

const float Epsilon = 1.0e-4;

// 13 taps in a 4x4 texel area: five overlapping 2x2 boxes (weights 0.5 for the center box, 0.125 for each corner box)
vec3 DownsampleBox13(sampler2D tex, vec2 uv, vec2 texelSize)
{
	vec3 A = texture(tex, uv + texelSize * vec2(-2.0,  2.0)).rgb;
	vec3 B = texture(tex, uv + texelSize * vec2( 0.0,  2.0)).rgb;
	vec3 C = texture(tex, uv + texelSize * vec2( 2.0,  2.0)).rgb;
	vec3 D = texture(tex, uv + texelSize * vec2(-2.0,  0.0)).rgb;
	vec3 E = texture(tex, uv).rgb;
	vec3 F = texture(tex, uv + texelSize * vec2( 2.0,  0.0)).rgb;
	vec3 G = texture(tex, uv + texelSize * vec2(-2.0, -2.0)).rgb;
	vec3 H = texture(tex, uv + texelSize * vec2( 0.0, -2.0)).rgb;
	vec3 I = texture(tex, uv + texelSize * vec2( 2.0, -2.0)).rgb;
	vec3 J = texture(tex, uv + texelSize * vec2(-1.0,  1.0)).rgb;
	vec3 K = texture(tex, uv + texelSize * vec2( 1.0,  1.0)).rgb;
	vec3 L = texture(tex, uv + texelSize * vec2(-1.0, -1.0)).rgb;
	vec3 M = texture(tex, uv + texelSize * vec2( 1.0, -1.0)).rgb;

	vec3 result = E * 0.125;
	result += (A + C + G + I) * 0.03125;
	result += (B + D + F + H) * 0.0625;
	result += (J + K + L + M) * 0.125;
	return result;
}

// Quadratic color thresholding with a soft knee; curve = (threshold - knee, knee * 2, 0.25 / knee)
vec3 QuadraticThreshold(vec3 color, float threshold, vec3 curve)
{
	float brightness = max(max(color.r, color.g), color.b);
	float rq = clamp(brightness - curve.x, 0.0, curve.y);
	rq = (rq * rq) * curve.z;
	return color * max(rq, brightness - threshold) / max(brightness, Epsilon);
}

// 3x3 tent filter (weights 1 2 1 / 2 4 2 / 1 2 1)
vec3 UpsampleTent9(sampler2D tex, vec2 uv, vec2 texelSize, float radius)
{
	vec4 offset = texelSize.xyxy * vec4(1.0, 1.0, -1.0, 0.0) * radius;

	vec3 result = texture(tex, uv).rgb * 4.0;

	result += texture(tex, uv - offset.xy).rgb;
	result += texture(tex, uv - offset.wy).rgb * 2.0;
	result += texture(tex, uv - offset.zy).rgb;

	result += texture(tex, uv + offset.zw).rgb * 2.0;
	result += texture(tex, uv + offset.xw).rgb * 2.0;

	result += texture(tex, uv + offset.zy).rgb;
	result += texture(tex, uv + offset.wy).rgb * 2.0;
	result += texture(tex, uv + offset.xy).rgb;

	return result * (1.0 / 16.0);
}

void main()
{
	vec2 uv = Input.TexCoord;
	vec3 color = vec3(0.0);

	if (u_Uniforms.Mode == MODE_PREFILTER)
	{
		vec2 texelSize = 1.0 / vec2(textureSize(u_Texture, 0));
		color = DownsampleBox13(u_Texture, uv, texelSize) * u_Uniforms.Exposure;
		color = min(color, vec3(20.0)); // very bright single pixels (the sun in an HDR map) would flicker as big blobs
		color = QuadraticThreshold(color, u_Uniforms.Params.x, u_Uniforms.Params.yzw);
	}
	else if (u_Uniforms.Mode == MODE_DOWNSAMPLE)
	{
		vec2 texelSize = 1.0 / vec2(textureSize(u_Texture, 0));
		color = DownsampleBox13(u_Texture, uv, texelSize);
	}
	else // MODE_UPSAMPLE
	{
		vec2 texelSize = 1.0 / vec2(textureSize(u_BloomTexture, 0));
		color = texture(u_Texture, uv).rgb + UpsampleTent9(u_BloomTexture, uv, texelSize, u_Uniforms.UpsampleScale);
	}

	o_Color = vec4(color, 1.0);
}
