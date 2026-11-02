// Ambient occlusion, step 3 (SceneEnvMapVulkan, see EnvMapVulkanSSAO and Include/SSAO.glslh): how much of the sky a
// surface point sees past the geometry close to it, from the depth buffer alone. Ground Truth Ambient Occlusion (Jimenez
// et al. 2016, as in Intel's XeGTAO).
//
// Per pixel, in view space (P the point, V towards the camera, N the normal): a few slices, each a plane through V
// turned around it by another angle. In a slice the depth buffer is walked away from the pixel in both directions, and on
// each side the highest thing seen from P is kept: the horizon, as the cosine of its angle to V. Between the two
// horizons the sky is visible; how much light that part lets in (with the cosine of the angle to the normal) has a
// closed form, so a slice costs its depth lookups and nothing else.
//
// The slices' angle and the steps' places differ from pixel to pixel and from frame to frame (the noise): the denoise and
// the temporal filter after this pass average them into more slices and steps than a pixel can afford.
// Written: 1 = open, 0 = closed in. Half the viewport's size.
#type compute
#version 450 core

#include "Include/SSAO.glslh"

layout (binding = 0) uniform sampler2D u_Depth;            // linear depth, with its smaller levels
layout (binding = 1) uniform sampler2D u_NormalRoughness;  // the G-buffer (full size): xyz = world normal, 0 = no surface
layout (binding = 2, r8) restrict writeonly uniform image2D o_AO;

const float PI = 3.14159265359;
const float HalfPI = 1.57079632679;
const float MaxRadiusPixels = 256.0; // full-size pixels: a point very close to the camera doesn't walk over the whole screen
const float MipOffset = 3.3;         // a step this many levels of two from its pixel reads the next smaller depth level

// Jimenez's interleaved gradient noise: a different value for every pixel, well spread in every small neighborhood
float InterleavedGradientNoise(vec2 pixel)
{
	return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
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
	vec3 worldNormal = texelFetch(u_NormalRoughness, FullPixel(pixel, fullSize), 0).xyz;
	if (dot(worldNormal, worldNormal) < 0.01)
	{
		imageStore(o_AO, pixel, vec4(1.0)); // no surface: the sky
		return;
	}

	vec2 uv = HalfPixelUV(pixel, fullSize);
	float depth = texelFetch(u_Depth, pixel, 0).r;
	vec3 P = ViewPosition(uv, depth);
	vec3 V = normalize(-P);
	vec3 N = normalize(mat3(u_SSAO.View) * worldNormal);
	if (dot(N, V) < -0.3)
	{
		N = -N; // the back of a two-sided mesh
	}

	// The radius on the screen: a view-space unit at this depth, in UV (with its direction) and in pixels of this image
	float radius = u_SSAO.Params.x;
	vec2 uvPerUnit = u_SSAO.Projection.xy / depth;
	float radiusPixels = radius * abs(uvPerUnit.x) * float(size.x);
	if (radiusPixels < 1.0)
	{
		imageStore(o_AO, pixel, vec4(1.0)); // too far away: the radius is within the pixel
		return;
	}
	float radiusScale = min(radiusPixels, MaxRadiusPixels / u_SSAO.Projection.w) / radiusPixels;
	radiusPixels *= radiusScale;

	// A sample stops counting towards the end of the radius (smoothly: no edge where it starts)
	float falloffRange = max(u_SSAO.Params.z, 0.01) * radius;
	float falloffScale = -1.0 / falloffRange;
	float falloffOffset = (radius - falloffRange) / falloffRange + 1.0;

	vec2 noisePixel = vec2(pixel) + u_SSAO.Params.w * 5.588238;
	float sliceNoise = InterleavedGradientNoise(noisePixel);
	float stepNoise = InterleavedGradientNoise(noisePixel + vec2(47.0, 17.0));

	int slices = u_SSAO.Counts.x;
	int steps = u_SSAO.Counts.y;
	float maxMip = float(u_SSAO.Counts.z - 1);
	float visibility = 0.0;
	for (int slice = 0; slice < slices; slice++)
	{
		float phi = (float(slice) + sliceNoise) / float(slices) * PI;
		vec3 direction = vec3(cos(phi), sin(phi), 0.0);
		// On the screen: to the end of the radius along the slice
		vec2 uvStep = direction.xy * uvPerUnit * radius * radiusScale;

		// The normal within the slice, as an angle to V (n), and how much of the normal is in the slice at all
		vec3 orthoDirection = direction - dot(direction, V) * V;
		vec3 axis = normalize(cross(orthoDirection, V));
		vec3 projectedNormal = N - axis * dot(N, axis);
		float projectedLength = max(length(projectedNormal), 0.0001);
		float cosNormal = clamp(dot(projectedNormal, V) / projectedLength, 0.0, 1.0);
		float n = sign(dot(orthoDirection, projectedNormal)) * acos(cosNormal);

		// The horizons start at the surface's own plane (nothing seen: the whole half circle over the normal is open)
		vec2 lowHorizonCos = vec2(cos(n + HalfPI), cos(n - HalfPI));
		vec2 horizonCos = lowHorizonCos;
		for (int j = 0; j < steps; j++)
		{
			// Closer together near the pixel, where the detail is; never within the pixel itself
			float t = (float(j) + stepNoise) / float(steps);
			t = t * t + 1.0 / radiusPixels;
			float mip = clamp(log2(t * radiusPixels) - MipOffset, 0.0, maxMip);
			for (int side = 0; side < 2; side++)
			{
				vec2 sampleUV = uv + (side == 0 ? t : -t) * uvStep;
				vec3 S = ViewPosition(sampleUV, textureLod(u_Depth, sampleUV, mip).r);
				vec3 toSample = S - P;
				float distance = length(toSample);
				float sampleCos = dot(toSample / max(distance, 0.00001), V);
				float weight = clamp(distance * falloffScale + falloffOffset, 0.0, 1.0);
				horizonCos[side] = max(horizonCos[side], mix(lowHorizonCos[side], sampleCos, weight));
			}
		}

		// The two horizons as angles from V, kept within the half circle over the normal
		float h0 = n + clamp(-acos(clamp(horizonCos[1], -1.0, 1.0)) - n, -HalfPI, HalfPI);
		float h1 = n + clamp(acos(clamp(horizonCos[0], -1.0, 1.0)) - n, -HalfPI, HalfPI);
		// The light the open part lets in, cosine-weighted (the closed form of the integral between h0 and h1)
		float arc0 = (cosNormal + 2.0 * h0 * sin(n) - cos(2.0 * h0 - n)) * 0.25;
		float arc1 = (cosNormal + 2.0 * h1 * sin(n) - cos(2.0 * h1 - n)) * 0.25;
		visibility += mix(projectedLength, 1.0, 0.05) * (arc0 + arc1);
	}
	visibility = pow(clamp(visibility / float(slices), 0.0, 1.0), u_SSAO.Params.y);
	// Fades in where the radius grows past a pixel: no line where the far-away cutoff above starts
	visibility = mix(1.0, visibility, clamp(radiusPixels - 1.0, 0.0, 1.0));
	imageStore(o_AO, pixel, vec4(visibility));
}
