// Probe update, step 6 (SceneEnvMapVulkan, see EnvMapVulkanProbes and Include/ProbeUpdate.glslh): where each probe of
// the batch stands. A regular grid ignores the scene, so some probes land inside walls or objects, and what they hold
// is of no use. Only in the first rounds after the meshes or the volume changed, with the same ray directions in every
// round, so a probe ends up in the same place whenever the scene is the same.
// The rays say where a probe is (a negative distance: the ray hit the back of a surface):
//   - more than a quarter of them hit backs: the probe is inside something. It moves along the ray with the nearest back,
//     through that surface and a little beyond. When that is farther than a probe may go, it is switched off: the
//     shaders that read the probes leave it out (Include/Probes.glslh), and it isn't updated any more.
//   - a surface's front is nearer than a probe should stand: the probe backs away from it.
// A probe stays within 45 percent of the probe spacing from its place in the grid, so it never leaves its cell.
// One thread per probe of the batch.
#type compute
#version 450 core

#include "Include/ProbeUpdate.glslh"

layout (binding = 0, rgba16f) restrict readonly uniform image2D u_RayBuffer;
// A texel per probe: xyz = its offset from the grid, w = 1 in use, 0 switched off. Every thread its own texel.
layout (binding = 1, rgba32f) restrict uniform image2D u_ProbeData;

const float BackfaceShare = 0.25;        // of a probe's rays: more than this, and the probe counts as inside something
const float MaxOffset = 0.45;            // of the probe spacing
const float BackfaceDistanceScale = 0.2; // a back's distance is stored as -0.2 x the distance (Include/ProbeCapture_Fragment.glslh)

// How far a probe at offset can go along a unit direction before it is MaxOffset of the spacing from its place in the
// grid (the limit is an ellipsoid when the spacing differs along the axes)
float ReachAlong(vec3 offset, vec3 direction)
{
	vec3 a = direction / u_Update.Relocation.xyz;
	vec3 b = offset / u_Update.Relocation.xyz;
	float aa = dot(a, a), ab = dot(a, b);
	float discriminant = ab * ab - aa * (dot(b, b) - MaxOffset * MaxOffset);
	return max((sqrt(max(discriminant, 0.0)) - ab) / aa, 0.0);
}

layout (local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
void main()
{
	int slot = int(gl_GlobalInvocationID.x);
	if (!BatchSlotUsed(slot))
	{
		return;
	}
	ivec2 texel = ProbeTile(BatchProbe(slot), u_Update.Counts.xyz);
	if (u_Update.RelocationMode.x == 0)
	{
		imageStore(u_ProbeData, texel, vec4(0.0, 0.0, 0.0, 1.0)); // relocation is off: on the grid, in use
		return;
	}
	// In the first round after a change every probe starts from the grid again (its rays were captured from there)
	vec3 offset = slot < u_Update.RelocationMode.y ? vec3(0.0) : imageLoad(u_ProbeData, texel).xyz;

	int backfaces = 0;
	float closestBack = 1.0e30, closestFront = 1.0e30;
	vec3 backDirection = vec3(0.0), frontDirection = vec3(0.0);
	for (int ray = 0; ray < u_Update.Batch.y; ray++)
	{
		float distance = imageLoad(u_RayBuffer, ivec2(ray, slot)).a;
		if (distance < 0.0)
		{
			backfaces++;
			distance /= -BackfaceDistanceScale;
			if (distance < closestBack)
			{
				closestBack = distance;
				backDirection = ProbeRayDirection(ray);
			}
		}
		else if (distance < closestFront)
		{
			closestFront = distance;
			frontDirection = ProbeRayDirection(ray);
		}
	}

	float minDistance = u_Update.Relocation.w;
	float inUse = 1.0;
	if (float(backfaces) > BackfaceShare * float(u_Update.Batch.y))
	{
		float reach = ReachAlong(offset, backDirection);
		if (reach > closestBack * 1.05 + 0.001)
		{
			offset += backDirection * min(closestBack + minDistance, reach);
		}
		else
		{
			inUse = 0.0; // the way out is too far
		}
	}
	else if (closestFront < minDistance)
	{
		offset -= frontDirection * min(minDistance - closestFront, ReachAlong(offset, -frontDirection));
	}
	imageStore(u_ProbeData, texel, vec4(offset, inUse));
}
