// Probe update, step 4 (SceneEnvMapVulkan, see EnvMapVulkanProbes and Include/ProbeUpdate.glslh): blends the rays of
// each probe of the batch into its visibility tile: how far the probe sees in each direction, as the mean distance and
// the mean squared distance of the rays near that direction (SampleProbeIrradiance in Include/Probes.glslh turns the
// two into how likely the probe sees a point).
// A group per probe of the batch (x) and quarter of its tile (y, 0 to 3), one thread per texel of the quarter (8 x 8):
// the tile has 16 x 16 texels, more threads than a group is guaranteed to have.
#type compute
#version 450 core

#include "Include/ProbeUpdate.glslh"

layout (binding = 0, rgba16f) restrict readonly uniform image2D u_RayBuffer;
layout (binding = 1, rg16f) restrict uniform image2D u_Visibility; // read and written in place: every thread its own texel

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	int slot = int(gl_WorkGroupID.x);
	ivec2 quarter = ivec2(int(gl_WorkGroupID.y) & 1, int(gl_WorkGroupID.y) >> 1);
	ivec2 interiorTexel = quarter * 8 + ivec2(gl_LocalInvocationID.xy) + 1; // after the border
	vec3 texelDirection = ProbeTexelDirection(interiorTexel, ProbeVisibilityTexels);

	vec2 sum = vec2(0.0);
	float weightSum = 0.0;
	for (int ray = 0; ray < u_Update.Batch.y; ray++)
	{
		// The back of a surface is stored as a short negative distance: the probe sees next to nothing that way
		float distance = min(abs(imageLoad(u_RayBuffer, ivec2(ray, slot)).a), u_Update.Params.z);
		// Only the rays close to the texel's direction count: a distance, unlike light, isn't spread over a hemisphere
		float weight = pow(max(dot(texelDirection, ProbeRayDirection(ray)), 0.0), 50.0);
		sum += vec2(distance, distance * distance) * weight;
		weightSum += weight;
	}
	if (weightSum < 0.0001)
	{
		return; // no ray near this direction in this update: the texel keeps its value
	}
	ivec2 texel = ProbeTileOrigin(BatchProbe(slot), ProbeVisibilityTexels) + interiorTexel;
	vec2 previous = imageLoad(u_Visibility, texel).rg;
	imageStore(u_Visibility, texel, vec4(mix(sum / weightSum, previous, u_Update.Params.x), 0.0, 0.0));
}
