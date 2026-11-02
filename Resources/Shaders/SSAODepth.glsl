// Ambient occlusion, step 1 (SceneEnvMapVulkan, see EnvMapVulkanSSAO and Include/SSAO.glslh): the scene's depth buffer
// into the largest level of the linear depth image, at half the viewport's size. A half-size pixel takes the depth of
// the full-size pixel it stands for (not a mix of four: the normal read for it later is that pixel's too).
#type compute
#version 450 core

#include "Include/SSAO.glslh"

layout (binding = 0) uniform sampler2D u_SceneDepth;
layout (binding = 1, r32f) restrict writeonly uniform image2D o_Depth;

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(pixel, imageSize(o_Depth))))
	{
		return;
	}
	ivec2 fullPixel = FullPixel(pixel, textureSize(u_SceneDepth, 0));
	imageStore(o_Depth, pixel, vec4(LinearDepth(texelFetch(u_SceneDepth, fullPixel, 0).r)));
}
