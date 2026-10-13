// Ray-traced reflections, the last step (SceneEnvMapVulkan, see EnvMapVulkanReflections): the traced reflections (one
// ray per pixel of the pass, averaged over the frames) to the viewport's size, into the image the mesh shaders read
// (u_ReflectionImage in set 0).
// A full-size pixel takes the traced pixels around it that belong to its surface: close in depth, with a similar
// normal. On a smooth surface only the nearest ones, so a mirror stays sharp; the rougher the surface, the wider the
// area, which is the rest of the blur a rough reflection needs (the rays spread it, this pass evens it out).
// Written: rgb = the reflected light, a = how much of it the mesh shaders take in place of the environment's reflection.
#type compute
#version 450 core

layout (binding = 0) uniform sampler2D u_Reflection;       // the pass's size: rgb = reflected light, a = weight
layout (binding = 1) uniform sampler2D u_Depth;            // the pass's size, linear
layout (binding = 2) uniform sampler2D u_SceneDepth;       // the depth buffer
layout (binding = 3) uniform sampler2D u_NormalRoughness;  // the G-buffer
layout (binding = 4, rgba16f) restrict writeonly uniform image2D o_Reflection;

// Must match EnvMapVulkanReflections::ResolvePush
layout (push_constant) uniform ReflectionResolvePush
{
	vec4 Params; // x = the max roughness, y = full-size pixels per pixel of the pass, z = 1: blur (0: the nearest traced
	             // pixels only), w = 0
	vec4 Depth;  // x = P[2][2], y = P[3][2], z = P[2][3]
} u_Resolve;

const float FadeStart = 0.75; // as in ReflectionTrace.glsl

float LinearDepth(float depth)
{
	float viewZ = u_Resolve.Depth.y / (depth * u_Resolve.Depth.z - u_Resolve.Depth.x);
	return viewZ * u_Resolve.Depth.z;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 fullSize = imageSize(o_Reflection);
	if (any(greaterThanEqual(pixel, fullSize)))
	{
		return;
	}
	vec4 normalRoughness = texelFetch(u_NormalRoughness, pixel, 0);
	float roughness = normalRoughness.w;
	float maxRoughness = u_Resolve.Params.x;
	if (dot(normalRoughness.xyz, normalRoughness.xyz) < 0.01 || roughness >= maxRoughness)
	{
		imageStore(o_Reflection, pixel, vec4(0.0));
		return;
	}
	vec3 normal = normalize(normalRoughness.xyz);
	float depth = LinearDepth(texelFetch(u_SceneDepth, pixel, 0).r);

	// The traced pixels around this one: the nearer in the image, in depth and in direction, the more each counts
	ivec2 passLast = textureSize(u_Reflection, 0) - 1;
	int scale = int(u_Resolve.Params.y);
	vec2 passPosition = vec2(pixel) / float(scale); // in traced pixels (a traced pixel stands for its first full-size pixel)
	ivec2 base = ivec2(floor(passPosition + 0.5));
	bool blur = u_Resolve.Params.z > 0.5;
	// How far the area reaches (in traced pixels) and how fast the weights fall within it
	float spread = blur ? roughness / maxRoughness : 0.0;
	int stride = 1 + int(spread * 2.0 + 0.5);
	float sigma = 0.6 + 1.2 * spread * float(stride);
	vec3 sum = vec3(0.0);
	float weights = 0.0;
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			ivec2 source = clamp(base + ivec2(x, y) * stride, ivec2(0), passLast);
			vec4 traced = texelFetch(u_Reflection, source, 0);
			if (traced.a <= 0.0)
			{
				continue; // no reflection was traced there
			}
			vec2 offset = vec2(source) - passPosition;
			float weight = exp(-dot(offset, offset) / (2.0 * sigma * sigma));
			weight *= 1.0 / (0.001 + abs(texelFetch(u_Depth, source, 0).r - depth) / depth * 50.0);
			vec3 sourceNormal = texelFetch(u_NormalRoughness, min(source * scale, fullSize - 1), 0).xyz;
			weight *= pow(max(dot(normal, normalize(sourceNormal + vec3(0.0, 0.00001, 0.0))), 0.0), 16.0) + 0.0001;
			sum += traced.rgb * weight;
			weights += weight;
		}
	}
	if (weights <= 0.0)
	{
		imageStore(o_Reflection, pixel, vec4(0.0)); // none around: the environment's reflection stays
		return;
	}
	float fade = 1.0 - smoothstep(FadeStart * maxRoughness, maxRoughness, roughness);
	imageStore(o_Reflection, pixel, vec4(sum / weights, fade));
}
