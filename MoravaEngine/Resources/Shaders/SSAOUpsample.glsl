// Ambient occlusion, the last step (SceneEnvMapVulkan, see EnvMapVulkanSSAO and Include/SSAO.glslh): the half-size
// result to the viewport's size, into the image the mesh shaders read (u_AOImage in set 0). With the passes at full
// size it only copies.
// A full-size pixel takes the four half-size values around it, each counted by how close it is and by how well its depth
// matches the pixel's own: at an object's edge the values of the surface behind it are left out, so the edge stays sharp.
#type compute
#version 450 core

#include "Include/SSAO.glslh"

layout (binding = 0) uniform sampler2D u_AO;          // half size
layout (binding = 1) uniform sampler2D u_Depth;       // half size, linear
layout (binding = 2) uniform sampler2D u_SceneDepth;  // the depth buffer
layout (binding = 3, r8) restrict writeonly uniform image2D o_AO;

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(pixel, imageSize(o_AO))))
	{
		return;
	}
	float sceneDepth = texelFetch(u_SceneDepth, pixel, 0).r;
	if (sceneDepth >= 1.0)
	{
		imageStore(o_AO, pixel, vec4(1.0)); // the sky
		return;
	}
	float depth = LinearDepth(sceneDepth);

	// A half-size pixel holds the first of its full-size pixels (FullPixel): this pixel is that one, or half way between
	// the half-size pixels base and base + 1. (At full size it is always that one: a copy.)
	ivec2 halfLast = textureSize(u_AO, 0) - 1;
	int scale = int(u_SSAO.Projection.w);
	ivec2 base = pixel / scale;
	vec2 between = vec2(pixel - base * scale) / float(scale);
	float sum = 0.0, weights = 0.0;
	for (int i = 0; i < 4; i++)
	{
		ivec2 offset = ivec2(i & 1, i >> 1);
		ivec2 source = clamp(base + offset, ivec2(0), halfLast);
		vec2 near = mix(1.0 - between, between, vec2(offset));
		float weight = near.x * near.y / (0.001 + abs(texelFetch(u_Depth, source, 0).r - depth) / depth);
		sum += texelFetch(u_AO, source, 0).r * weight;
		weights += weight;
	}
	imageStore(o_AO, pixel, vec4(weights > 0.0 ? sum / weights : 1.0));
}
