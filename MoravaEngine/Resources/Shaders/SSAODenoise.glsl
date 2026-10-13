// Ambient occlusion, step 4 (SceneEnvMapVulkan, see EnvMapVulkanSSAO and Include/SSAO.glslh): the noise of the GTAO
// pass blurred away, over 5 x 5 pixels. A neighbor counts only as far as it belongs to the same surface: by how far it is
// from the pixel's own plane (its position against the pixel's position and normal) and by how much its normal differs.
// So the blur never crosses an edge: an object doesn't get the wall's light value behind it, or a dark halo on that wall.
// u_SSAO.Counts.w = 0: no blur, the value is passed on as it is.
#type compute
#version 450 core

#include "Include/SSAO.glslh"

layout (binding = 0) uniform sampler2D u_AO;
layout (binding = 1) uniform sampler2D u_Depth;
layout (binding = 2) uniform sampler2D u_NormalRoughness;
layout (binding = 3, r8) restrict writeonly uniform image2D o_AO;

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(o_AO);
	if (any(greaterThanEqual(pixel, size)))
	{
		return;
	}
	float center = texelFetch(u_AO, pixel, 0).r;
	ivec2 fullSize = textureSize(u_NormalRoughness, 0);
	vec3 worldNormal = texelFetch(u_NormalRoughness, FullPixel(pixel, fullSize), 0).xyz;
	if (u_SSAO.Counts.w == 0 || dot(worldNormal, worldNormal) < 0.01)
	{
		imageStore(o_AO, pixel, vec4(center));
		return;
	}
	worldNormal = normalize(worldNormal);
	float depth = texelFetch(u_Depth, pixel, 0).r;
	vec3 P = ViewPosition(HalfPixelUV(pixel, fullSize), depth);
	vec3 N = mat3(u_SSAO.View) * worldNormal;
	// How far off the pixel's plane a neighbor may be: more with the distance, where a pixel covers more of the surface
	float planeTolerance = 0.02 + 0.01 * depth;

	const float gauss[3] = float[3](1.0, 0.7, 0.3);
	float sum = 0.0, weights = 0.0;
	for (int y = -2; y <= 2; y++)
	{
		for (int x = -2; x <= 2; x++)
		{
			ivec2 neighbor = clamp(pixel + ivec2(x, y), ivec2(0), size - 1);
			vec3 neighborNormal = texelFetch(u_NormalRoughness, FullPixel(neighbor, fullSize), 0).xyz;
			vec3 S = ViewPosition(HalfPixelUV(neighbor, fullSize), texelFetch(u_Depth, neighbor, 0).r);
			float weight = gauss[abs(x)] * gauss[abs(y)];
			weight *= clamp(1.0 - abs(dot(N, S - P)) / planeTolerance, 0.0, 1.0);
			float alike = max(dot(worldNormal, neighborNormal), 0.0); // 0 for the sky (no normal)
			weight *= alike * alike * alike * alike;
			sum += texelFetch(u_AO, neighbor, 0).r * weight;
			weights += weight;
		}
	}
	imageStore(o_AO, pixel, vec4(weights > 0.0001 ? sum / weights : center));
}
