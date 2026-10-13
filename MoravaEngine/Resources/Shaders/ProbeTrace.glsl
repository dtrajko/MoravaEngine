// Probe update with ray tracing (SceneEnvMapVulkan, see EnvMapVulkanProbes and Phase H.1 of
// docs/rendering/SSAO_GI_RayTracing_Guide.html): fills the ray buffer by tracing each probe's rays against the scene.
// It replaces the cube map capture and ProbeResample.glsl, and writes exactly what they write, so the passes that blend
// the rays into the atlases and place the probes don't change.
// What a ray brings back is the light the surface it hits sends towards the probe, as a matte surface
// (ShadeSurfaceDiffuse, the function the capture uses): the lights, what the surface emits, and the light the probes
// already hold, which is how light bounces more than once. Compared with the capture:
//   - the lights' shadows are shadow rays, so there is no shadow map to run out of (the capture needs its own map of the
//     sun for the whole volume);
//   - the surface's color comes from its maps (read at a level as coarse as the ray is wide), not from their averages;
//   - no draw calls: a probe costs its rays, whatever the number of meshes around it.
// A ray that hits nothing gets the environment's light; one that hits a surface from behind brings no light and a
// negative distance (the probe is probably inside a mesh).
// Sets: 0 the per-frame set's contents (lights, environment, probes: Include/FrameSet.glslh), 1 the ray buffer, 3 the
// ray tracing set. One thread per ray (x) and slot (y).
#type compute
#version 460 core

#include "Include/RayTracing.glslh"
#include "Include/ProbeUpdate.glslh"

// The lights' shadows by rays, and the probes' light whether or not they light the scene (Light the Scene)
float ProbeSunShadow(vec3 position, vec3 geometryNormal);
float ProbeLightShadow(vec3 position, vec3 geometryNormal, vec3 L, float distance);
#define SHADE_SURFACE_SUN_SHADOW ProbeSunShadow
#define SHADE_SURFACE_LIGHT_SHADOW ProbeLightShadow
#define SHADE_SURFACE_PROBES_ALWAYS
#include "Include/ShadeSurface.glslh"

layout (set = 1, binding = 0, rgba16f) restrict writeonly uniform image2D o_RayBuffer;

// As in Include/ProbeCapture_Fragment.glslh: a surface never sends back all the light it gets (the light bouncing
// between the probes dies out), and one ray on a small, very bright spot must not flicker in the probe
const float MaxBounceAlbedo = 0.9;
const float MaxRadiance = 100.0;
const float SunDistance = 10000.0;

const int TraceSunShadows = 1; // bits of u_Update.Batch.w
const int TracePlacing = 2;

// 1 when nothing stands between the point and the light, 0 in its shadow
float ProbeLightShadow(vec3 position, vec3 geometryNormal, vec3 L, float distance)
{
	return TraceShadowRay(RayOriginOffSurface(position, geometryNormal), L, 0.0, distance, RayMaskShadowCasters) ? 0.0 : 1.0;
}

float ProbeSunShadow(vec3 position, vec3 geometryNormal)
{
	if ((u_Update.Batch.w & TraceSunShadows) == 0)
	{
		return 1.0;
	}
	return ProbeLightShadow(position, geometryNormal, u_Sun.Direction, SunDistance);
}

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	int ray = int(gl_GlobalInvocationID.x);
	int slot = int(gl_GlobalInvocationID.y);
	if (ray >= u_Update.Batch.y || !BatchSlotUsed(slot))
	{
		return;
	}
	bool placing = (u_Update.Batch.w & TracePlacing) != 0;

	// Where the probe stands: on the grid, moved by its offset. While the probes are placed, where ProbeRelocate.glsl
	// takes it to stand: on the grid in the first round (and whenever relocation is off).
	ivec3 probe = BatchProbe(slot);
	vec4 probeData = ProbeData(probe);
	if (placing && (slot < u_Update.RelocationMode.y || u_Update.RelocationMode.x == 0))
	{
		probeData.xyz = vec3(0.0);
	}
	vec3 origin = ProbePosition(probe, probeData);
	vec3 direction = ProbeRayDirection(ray);

	// The probes are placed by what stands still: a probe isn't moved out of a model that walks on
	RayHit hit = TraceRay(origin, direction, 0.0, ProbeMissDistance * 0.5, placing ? RayMaskOpaqueStatic : RayMaskOpaque);
	if (!hit.Hit)
	{
		// The sky. A ray stands for its share of the sphere, so it reads the environment at the mip level whose texels are
		// about that large: a sharp lookup would make a small bright sun flicker in and out of the rays.
		float faceTexels = float(textureSize(u_EnvRadianceTex, 0).x);
		float lod = max(0.5 * log2(6.0 * faceTexels * faceTexels / float(u_Update.Batch.y)), 0.0);
		vec3 sky = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, direction), lod).rgb;
		imageStore(o_RayBuffer, ivec2(ray, slot), vec4(sky, ProbeMissDistance));
		return;
	}

	RaySurface raySurface = GetRaySurface(hit);
	vec3 normal = normalize(raySurface.Normal);
	if (dot(normal, direction) > 0.0)
	{
		imageStore(o_RayBuffer, ivec2(ray, slot), vec4(0.0, 0.0, 0.0, -0.2 * hit.T)); // seen from behind
		return;
	}

	// The ray stands for its share of the sphere around the probe: that wide where it hits
	float footprint = hit.T * sqrt(12.566 / float(u_Update.Batch.y));
	RayMaterial material = GetRayMaterial(raySurface, footprint);

	SurfacePoint surface;
	surface.Position = raySurface.Position;
	surface.Normal = normal;
	surface.ToViewer = -direction;
	surface.Albedo = min(material.Albedo, vec3(MaxBounceAlbedo));
	surface.Metalness = material.Metalness;
	surface.Emissive = material.Emissive;
	vec3 radiance = min(ShadeSurfaceDiffuse(surface, true), vec3(MaxRadiance));
	imageStore(o_RayBuffer, ivec2(ray, slot), vec4(radiance, hit.T));
}
