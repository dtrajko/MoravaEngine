// Ambient occlusion by ray tracing (SceneEnvMapVulkan, see EnvMapVulkanSSAO, Include/SSAO.glslh and Phase H.2 of
// docs/rendering/SSAO_GI_RayTracing_Guide.html): takes the place of SSAOGTAO.glsl when rays can be traced. It writes the
// same image (1 = open, 0 = closed in, at the passes' size), so the blur, the temporal filter and the upsample after it
// don't change.
//
// Per pixel: a few short rays from the surface point into the half sphere over its normal, more of them near the normal
// (cosine-weighted: as the light from all around counts). A ray that hits something within the radius is light that
// doesn't arrive. The screen-space pass can only guess from the depth buffer; a ray meets the real geometry, also where
// it is hidden behind something else or outside the view.
//
// A pixel gets other directions than its neighbors and other ones in every frame (the noise): the blur and the temporal
// filter average them into many more rays than a pixel traces.
// Sets: 0 the pass's own (as SSAOGTAO.glsl's), 3 the ray tracing set.
#type compute
#version 460 core

#include "Include/RayTracing.glslh"
#include "Include/SSAO.glslh"

layout (set = 0, binding = 0) uniform sampler2D u_Depth;            // linear depth, at the passes' size
layout (set = 0, binding = 1) uniform sampler2D u_NormalRoughness;  // the G-buffer (full size): xyz = world normal, 0 = no surface
layout (set = 0, binding = 2, r8) restrict writeonly uniform image2D o_AO;

const float PI = 3.14159265359;

// A well-mixed number from a number (PCG), and floats in 0..1 drawn one after the other from a state
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

// The view-space position of a pixel of the passes
vec3 PixelViewPosition(ivec2 pixel, ivec2 size, ivec2 fullSize)
{
	pixel = clamp(pixel, ivec2(0), size - 1);
	return ViewPosition(HalfPixelUV(pixel, fullSize), texelFetch(u_Depth, pixel, 0).r);
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(o_AO);
	if (any(greaterThanEqual(pixel, size)))
	{
		return;
	}
	ivec2 fullSize = textureSize(u_NormalRoughness, 0);
	vec3 normal = texelFetch(u_NormalRoughness, FullPixel(pixel, fullSize), 0).xyz;
	if (dot(normal, normal) < 0.01)
	{
		imageStore(o_AO, pixel, vec4(1.0)); // no surface: the sky
		return;
	}
	normal = normalize(normal);

	// The surface point, and the surface's own normal from the depths around it (the G-buffer's normal is the shading
	// normal: bent by the normal map and smoothed over the triangles). On each axis the neighbor closer in depth is
	// taken: the other one may belong to another surface.
	float depth = texelFetch(u_Depth, pixel, 0).r;
	vec3 viewPosition = PixelViewPosition(pixel, size, fullSize);
	vec3 left = viewPosition - PixelViewPosition(pixel - ivec2(1, 0), size, fullSize);
	vec3 right = PixelViewPosition(pixel + ivec2(1, 0), size, fullSize) - viewPosition;
	vec3 up = viewPosition - PixelViewPosition(pixel - ivec2(0, 1), size, fullSize);
	vec3 down = PixelViewPosition(pixel + ivec2(0, 1), size, fullSize) - viewPosition;
	vec3 alongX = abs(left.z) < abs(right.z) ? left : right;
	vec3 alongY = abs(up.z) < abs(down.z) ? up : down;
	vec3 viewGeometryNormal = cross(alongX, alongY);
	viewGeometryNormal = dot(viewGeometryNormal, viewGeometryNormal) > 0.0 ? normalize(viewGeometryNormal) : vec3(0.0);
	if (dot(viewGeometryNormal, viewPosition) > 0.0)
	{
		viewGeometryNormal = -viewGeometryNormal; // towards the camera
	}

	// Into world space: the view matrix is a rotation and a translation, so its inverse is the transposed rotation
	mat3 viewToWorld = transpose(mat3(u_SSAO.View));
	vec3 position = viewToWorld * (viewPosition - u_SSAO.View[3].xyz);
	vec3 geometryNormal = viewToWorld * viewGeometryNormal;
	vec3 toCamera = normalize(viewToWorld * -viewPosition);
	if (dot(geometryNormal, geometryNormal) < 0.5)
	{
		geometryNormal = normal;
	}
	if (dot(normal, toCamera) < -0.3)
	{
		normal = -normal; // the back of a two-sided mesh
	}

	// The rays start a little off the surface: the position comes from the depth buffer, which gets coarser with the distance
	vec3 origin = position + geometryNormal * (0.001 + 0.0005 * depth + 0.00002 * depth * depth);

	// A hit stops counting towards the end of the radius (smoothly: no edge where it starts), as in SSAOGTAO.glsl
	float radius = u_SSAO.Params.x;
	float falloffRange = max(u_SSAO.Params.z, 0.01) * radius;

	// A frame around the normal
	vec3 tangent = normalize(abs(normal.y) < 0.99 ? cross(normal, vec3(0.0, 1.0, 0.0)) : cross(normal, vec3(1.0, 0.0, 0.0)));
	vec3 bitangent = cross(normal, tangent);

	uint random = Hash(uint(pixel.x) + Hash(uint(pixel.y) + Hash(uint(u_SSAO.Params.w))));
	int rays = max(u_SSAO.Counts.x, 1);
	float occlusion = 0.0;
	for (int ray = 0; ray < rays; ray++)
	{
		// Cosine-weighted: a point evenly on a disc, lifted onto the half sphere
		float r = sqrt(NextRandom(random));
		float phi = 2.0 * PI * NextRandom(random);
		vec3 direction = normalize(tangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + normal * sqrt(max(1.0 - r * r, 0.0)));
		// A direction over the shading normal can point into the surface itself: mirrored back out of it
		float outOfSurface = dot(direction, geometryNormal);
		if (outOfSurface < 0.0)
		{
			direction -= 2.0 * outOfSurface * geometryNormal;
		}

		float hitDistance = TraceRayDistance(origin, direction, 0.0, radius, RayMaskOpaque);
		if (hitDistance >= 0.0)
		{
			occlusion += clamp((radius - hitDistance) / falloffRange, 0.0, 1.0);
		}
	}
	float visibility = pow(clamp(1.0 - occlusion / float(rays), 0.0, 1.0), u_SSAO.Params.y);
	imageStore(o_AO, pixel, vec4(visibility));
}
