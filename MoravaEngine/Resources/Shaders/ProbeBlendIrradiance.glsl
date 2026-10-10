// Probe update, step 3 (SceneEnvMapVulkan, see EnvMapVulkanProbes and Include/ProbeUpdate.glslh): blends the rays of
// each probe of the batch into its irradiance tile. A texel stands for a direction: it gets the light of all rays,
// each counted by the cosine of its angle to that direction (the light a surface facing that way receives, divided by
// pi), mixed with what the texel held before.
// One group per probe of the batch, one thread per texel of its tile (8 x 8).
#type compute
#version 450 core

#include "Include/ProbeUpdate.glslh"

layout (binding = 0, rgba16f) restrict readonly uniform image2D u_RayBuffer;
layout (binding = 1, rgba16f) restrict uniform image2D u_Irradiance; // read and written in place: every thread its own texel

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	int slot = int(gl_WorkGroupID.x);
	ivec2 interiorTexel = ivec2(gl_LocalInvocationID.xy) + 1; // after the border
	vec3 texelDirection = ProbeTexelDirection(interiorTexel, ProbeIrradianceTexels);

	vec3 sum = vec3(0.0);
	float weightSum = 0.0;
	for (int ray = 0; ray < u_Update.Batch.y; ray++)
	{
		vec4 rayValue = imageLoad(u_RayBuffer, ivec2(ray, slot));
		if (rayValue.a < 0.0)
		{
			continue; // the back of a surface: the probe is inside something there, what it sees doesn't count
		}
		float weight = max(dot(texelDirection, ProbeRayDirection(ray)), 0.0);
		sum += rayValue.rgb * weight;
		weightSum += weight;
	}
	vec3 irradiance = weightSum > 0.0001 ? sum / weightSum : vec3(0.0);

	// Stored to the power 1 / gamma (see SampleProbeIrradiance in Include/Probes.glslh)
	float gamma = u_Update.Params.y;
	ivec2 texel = ProbeTileOrigin(BatchProbe(slot), ProbeIrradianceTexels) + interiorTexel;
	vec3 previousStored = imageLoad(u_Irradiance, texel).rgb;
	vec3 stored = pow(max(irradiance, vec3(0.0)), vec3(1.0 / gamma));

	// A large change (a light switched on or off) is taken over quickly; small ones, the noise of few rays, slowly
	float hysteresis = u_Update.Params.x;
	vec3 change = abs(stored - previousStored);
	if (max(change.r, max(change.g, change.b)) > 0.25)
	{
		hysteresis = max(hysteresis - 0.75, 0.0);
	}
	vec3 blended = mix(irradiance, pow(previousStored, vec3(gamma)), hysteresis);
	imageStore(u_Irradiance, texel, vec4(pow(max(blended, vec3(0.0)), vec3(1.0 / gamma)), 1.0));
}
