// Ambient occlusion, step 2 (SceneEnvMapVulkan, see EnvMapVulkanSSAO and Include/SSAO.glslh): a smaller level of the
// linear depth image from the level above it (u_SSAO.Counts.w), one dispatch per level. Samples far from their pixel read
// the small levels: fewer texels, so the lookups stay in the cache.
// A texel is the average of its four, without those far in front of the farthest one: a thin object in front of a wall
// would otherwise drag the wall forward, and darken everything around it.
#type compute
#version 450 core

#include "Include/SSAO.glslh"

layout (binding = 0) uniform sampler2D u_Depth;                         // the whole chain: the level above is read
layout (binding = 1, r32f) restrict writeonly uniform image2D o_Depth;  // the level written

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(pixel, imageSize(o_Depth))))
	{
		return;
	}
	int source = u_SSAO.Counts.w;
	ivec2 last = textureSize(u_Depth, source) - 1;
	float depths[4];
	float farthest = 0.0;
	for (int i = 0; i < 4; i++)
	{
		depths[i] = texelFetch(u_Depth, min(pixel * 2 + ivec2(i & 1, i >> 1), last), source).r;
		farthest = max(farthest, depths[i]);
	}
	float range = 0.75 * u_SSAO.Params.x; // of the occlusion's radius
	float sum = 0.0, weights = 0.0;
	for (int i = 0; i < 4; i++)
	{
		float weight = clamp(1.0 - (farthest - depths[i]) / range, 0.0, 1.0);
		sum += depths[i] * weight;
		weights += weight;
	}
	imageStore(o_Depth, pixel, vec4(sum / weights)); // the farthest one's weight is 1
}
