// Ray-traced reflections, step 1 (SceneEnvMapVulkan, see EnvMapVulkanReflections and Phase H.3 of
// docs/rendering/SSAO_GI_RayTracing_Guide.html): what each pixel's surface reflects, found by a ray.
// The mesh shaders take their reflections from the environment map, which shows the sky and nothing of the scene: a room
// reflects the sky through its walls. Here a ray leaves the surface the way a reflection does and brings back what it
// really meets: another surface, lit, or the sky when nothing is in the way.
//
// Per pixel of the pass (from the G-buffer: depth, normal, roughness):
//   - the direction: mirrored at the normal on a smooth surface; on a rougher one mirrored at a tiny facet whose tilt
//     is drawn from the surface's roughness (the GGX distribution, only the facets the viewer can see), a different
//     one for every pixel and frame. Averaged over pixels and frames (the passes after this one) that is the blurred
//     reflection of a rough surface.
//   - what the ray hits is lit as a matte surface (ShadeSurfaceDiffuse: the lights with shadow rays, what it emits, the
//     probes' light), plus the probes' light from the side it would mirror, as its own blurry reflection (a metal has
//     no matte light at all). A reflection in a reflection is not followed.
//   - a ray that hits nothing gets the environment's light, as the mesh shaders read it.
// A surface rougher than the Max Roughness setting gets no ray (its reflection is so blurred that the environment's is
// close enough, and its rays would be all noise): weight 0, and the mesh shaders keep the environment's reflection.
// Written: rgb = the reflected light, a = the weight (1: use it; falling to 0 towards the max roughness), and the
// pixel's linear depth for the passes after this one.
// Sets: 0 the per-frame set's contents, 1 the pass's own, 3 the ray tracing set.
#type compute
#version 460 core

#include "Include/RayTracing.glslh"

// Must match EnvMapVulkanReflections::TracePush
layout (push_constant) uniform ReflectionTracePush
{
	mat4 InverseViewProjection; // offset 0:   clip space to world space
	vec4 CameraPosition;        // offset 64:  xyz; w = this frame's noise number
	vec4 Params;                // offset 80:  x = the max roughness, y = full-size pixels per pixel of the pass (along an
	                            //             axis), z = 1: the sun casts shadows, w = the angle a full-size pixel covers
	vec4 Depth;                 // offset 96:  x = P[2][2], y = P[3][2], z = P[2][3] (the projection's depth terms)
} u_Trace;

#define RAY_TRACED_SUN_SHADOWS (u_Trace.Params.z > 0.5)
#include "Include/ShadeSurfaceRayTraced.glslh"

layout (set = 1, binding = 0) uniform sampler2D u_SceneDepth;       // the depth buffer
layout (set = 1, binding = 1) uniform sampler2D u_NormalRoughness;  // the G-buffer: xyz = world normal (0: no surface), w = roughness
layout (set = 1, binding = 2, rgba16f) restrict writeonly uniform image2D o_Reflection;
layout (set = 1, binding = 3, r32f) restrict writeonly uniform image2D o_Depth;

const float PI = 3.14159265359;
const float MaxRadiance = 50.0;      // brighter is cut off: one ray on a small, very bright spot would sparkle
const float MissDistance = 10000.0;
const float FadeStart = 0.75;        // of the max roughness: from here the weight falls to 0

uint Hash(uint value)
{
	uint state = value * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float NextRandom(inout uint state)
{
	state = Hash(state);
	return float(state) * (1.0 / 4294967296.0);
}

// The depth buffer's value as the distance from the camera along its view direction
float LinearDepth(float depth)
{
	float viewZ = u_Trace.Depth.y / (depth * u_Trace.Depth.z - u_Trace.Depth.x);
	return viewZ * u_Trace.Depth.z;
}

// The world position of a full-size pixel's surface point
vec3 PixelPosition(ivec2 fullPixel, ivec2 fullSize)
{
	fullPixel = clamp(fullPixel, ivec2(0), fullSize - 1);
	vec2 ndc = (vec2(fullPixel) + 0.5) / vec2(fullSize) * 2.0 - 1.0;
	vec4 world = u_Trace.InverseViewProjection * vec4(ndc, texelFetch(u_SceneDepth, fullPixel, 0).r, 1.0);
	return world.xyz / world.w;
}

// A facet normal of a GGX surface (alpha: its roughness squared) among those visible from V, in the frame where the
// surface normal is z (Heitz 2018, "Sampling the GGX Distribution of Visible Normals")
vec3 SampleVisibleFacet(vec3 V, float alpha, float u1, float u2)
{
	vec3 stretched = normalize(vec3(alpha * V.x, alpha * V.y, V.z));
	float lengthSquared = stretched.x * stretched.x + stretched.y * stretched.y;
	vec3 t1 = lengthSquared > 0.0 ? vec3(-stretched.y, stretched.x, 0.0) / sqrt(lengthSquared) : vec3(1.0, 0.0, 0.0);
	vec3 t2 = cross(stretched, t1);
	float r = sqrt(u1);
	float phi = 2.0 * PI * u2;
	float p1 = r * cos(phi);
	float p2 = r * sin(phi);
	float s = 0.5 * (1.0 + stretched.z);
	p2 = (1.0 - s) * sqrt(max(1.0 - p1 * p1, 0.0)) + s * p2;
	vec3 facet = p1 * t1 + p2 * t2 + sqrt(max(1.0 - p1 * p1 - p2 * p2, 0.0)) * stretched;
	return normalize(vec3(alpha * facet.x, alpha * facet.y, max(facet.z, 0.0)));
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(o_Reflection);
	if (any(greaterThanEqual(pixel, size)))
	{
		return;
	}
	// A pixel of the pass stands for the first of the full-size pixels it covers
	ivec2 fullSize = textureSize(u_NormalRoughness, 0);
	int scale = int(u_Trace.Params.y);
	ivec2 fullPixel = min(pixel * scale, fullSize - 1);
	float depth = LinearDepth(texelFetch(u_SceneDepth, fullPixel, 0).r);
	imageStore(o_Depth, pixel, vec4(depth));

	vec4 normalRoughness = texelFetch(u_NormalRoughness, fullPixel, 0);
	float roughness = normalRoughness.w;
	float maxRoughness = u_Trace.Params.x;
	if (dot(normalRoughness.xyz, normalRoughness.xyz) < 0.01 || roughness >= maxRoughness)
	{
		imageStore(o_Reflection, pixel, vec4(0.0)); // the sky, or too rough: no reflection from here
		return;
	}
	float weight = 1.0 - smoothstep(FadeStart * maxRoughness, maxRoughness, roughness);

	vec3 position = PixelPosition(fullPixel, fullSize);
	vec3 V = normalize(u_Trace.CameraPosition.xyz - position);
	vec3 N = normalize(normalRoughness.xyz);
	if (dot(N, V) < 0.0)
	{
		N = -N; // the back of a two-sided mesh
	}

	// The surface's own normal, from the positions around the pixel (the G-buffer's is the shading normal, bent by the
	// normal map). On each axis the neighbor closer to the pixel is taken: the other may belong to another surface.
	vec3 left = position - PixelPosition(fullPixel - ivec2(scale, 0), fullSize);
	vec3 right = PixelPosition(fullPixel + ivec2(scale, 0), fullSize) - position;
	vec3 up = position - PixelPosition(fullPixel - ivec2(0, scale), fullSize);
	vec3 down = PixelPosition(fullPixel + ivec2(0, scale), fullSize) - position;
	vec3 geometryNormal = cross(dot(left, left) < dot(right, right) ? left : right, dot(up, up) < dot(down, down) ? up : down);
	geometryNormal = dot(geometryNormal, geometryNormal) > 0.0 ? normalize(geometryNormal) : N;
	if (dot(geometryNormal, V) < 0.0)
	{
		geometryNormal = -geometryNormal;
	}

	// The reflection's direction: V mirrored at a facet of the rough surface
	vec3 tangent = normalize(abs(N.y) < 0.99 ? cross(N, vec3(0.0, 1.0, 0.0)) : cross(N, vec3(1.0, 0.0, 0.0)));
	vec3 bitangent = cross(N, tangent);
	uint random = Hash(uint(pixel.x) + Hash(uint(pixel.y) + Hash(uint(u_Trace.CameraPosition.w))));
	float alpha = roughness * roughness;
	vec3 localV = vec3(dot(V, tangent), dot(V, bitangent), dot(V, N));
	vec3 localFacet = SampleVisibleFacet(localV, alpha, NextRandom(random), NextRandom(random));
	vec3 facet = tangent * localFacet.x + bitangent * localFacet.y + N * localFacet.z;
	vec3 direction = reflect(-V, facet);
	// A direction that points into the surface itself (a bent shading normal, a steep facet): mirrored back out of it
	float outOfSurface = dot(direction, geometryNormal);
	if (outOfSurface < 0.0)
	{
		direction -= 2.0 * outOfSurface * geometryNormal;
	}

	// The ray starts a little off the surface: the position comes from the depth buffer, which gets coarser with the distance
	vec3 origin = position + geometryNormal * (0.001 + 0.0005 * depth + 0.00002 * depth * depth);
	RayHit hit = TraceRay(origin, direction, 0.0, MissDistance, RayMaskOpaque);
	vec3 radiance;
	if (!hit.Hit)
	{
		// The sky, blurred by the roughness as the mesh shaders read it (IndirectSpecular in Include/PBRCommon.glslh)
		float levels = float(textureQueryLevels(u_EnvRadianceTex));
		radiance = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, direction), roughness * levels).rgb;
	}
	else
	{
		// The maps at the level that fits how wide the reflection is where it hits: a pixel's width at that distance,
		// and the spread of the rough surface's rays
		RaySurface raySurface = GetRaySurface(hit);
		float footprint = (length(position - u_Trace.CameraPosition.xyz) + hit.T) * u_Trace.Params.w * float(scale) + hit.T * alpha;
		RayMaterial material = GetRayMaterial(raySurface, footprint);
		vec3 hitNormal = dot(material.Normal, direction) > 0.0 ? -material.Normal : material.Normal;

		SurfacePoint surface;
		surface.Position = raySurface.Position;
		surface.Normal = hitNormal;
		surface.ToViewer = -direction;
		surface.Albedo = material.Albedo;
		surface.Metalness = material.Metalness;
		surface.Emissive = material.Emissive;
		radiance = ShadeSurfaceDiffuse(surface, true);
		// Its own reflection, as blurry as the probes are: the light arriving from the side it mirrors
		vec3 F0 = mix(vec3(0.04), material.Albedo, material.Metalness);
		radiance += F0 * IndirectDiffuse(raySurface.Position, reflect(direction, hitNormal), -direction);
	}
	imageStore(o_Reflection, pixel, vec4(min(radiance, vec3(MaxRadiance)), weight));
}
