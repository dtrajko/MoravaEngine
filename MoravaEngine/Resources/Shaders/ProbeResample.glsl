// Probe update, step 2 (SceneEnvMapVulkan, see EnvMapVulkanProbes and Include/ProbeUpdate.glslh): fills the ray buffer
// from the cube maps captured around the probes of the batch. Every ray reads the cube of its probe in its direction:
// what it would have hit, and how far away. A ray that hit nothing gets the environment's light.
// The only compute pass that knows about the cubes: ray tracing replaces the capture and this pass, and writes the same
// buffer.
// One thread per ray (x) and slot (y).
#type compute
#version 450 core

#include "Include/ProbeUpdate.glslh"

layout (binding = 0) uniform samplerCubeArray u_CaptureRadiance; // a cube per slot: rgb = the light a surface sends to the probe
layout (binding = 1) uniform samplerCubeArray u_CaptureDistance; // r = its distance from the probe (negative: its back), ProbeMissDistance: none
layout (binding = 2) uniform samplerCube u_EnvRadianceTex;
layout (binding = 3, rgba16f) restrict writeonly uniform image2D o_RayBuffer;

// As in Include/ShadeSurface.glslh: the environment lookups are turned by the environment's rotation
vec3 RotateVectorAboutY(float angle, vec3 vec)
{
	angle = radians(angle);
	mat3x3 rotationMatrix = { vec3(cos(angle), 0.0, sin(angle)),
	                          vec3(0.0, 1.0, 0.0),
	                          vec3(-sin(angle), 0.0, cos(angle)) };
	return rotationMatrix * vec;
}

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	int ray = int(gl_GlobalInvocationID.x);
	int slot = int(gl_GlobalInvocationID.y);
	if (ray >= u_Update.Batch.y || slot >= u_Update.Batch.x)
	{
		return;
	}
	vec3 direction = ProbeRayDirection(ray);
	float distance = textureLod(u_CaptureDistance, vec4(direction, float(slot)), 0.0).r;
	vec3 radiance;
	if (distance >= ProbeMissDistance * 0.5)
	{
		// The sky. A ray stands for its share of the sphere, so it reads the environment at the mip level whose texels are
		// about that large: a sharp lookup would make a small bright sun flicker in and out of the rays.
		float faceTexels = float(textureSize(u_EnvRadianceTex, 0).x);
		float lod = max(0.5 * log2(6.0 * faceTexels * faceTexels / float(u_Update.Batch.y)), 0.0);
		radiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_Update.Params.w, direction), lod).rgb;
		distance = ProbeMissDistance;
	}
	else
	{
		radiance = textureLod(u_CaptureRadiance, vec4(direction, float(slot)), 0.0).rgb;
	}
	imageStore(o_RayBuffer, ivec2(ray, slot), vec4(radiance, distance));
}
