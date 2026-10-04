// The water's wireframe (Water panel, Wireframe): the grid of the water surface as lines, where the Gerstner waves move it
// this frame, drawn over the surface (a line-mode pipeline with a depth bias toward the camera, see EnvMapVulkanWater).
// Descriptor sets: declared exactly as in Water.glsl, so the water's sets are valid here.
#type vertex
#version 450 core

#include "Include/WaterVertex.glslh"

#type fragment
#version 450 core

layout (location = 0) in vec3 v_WorldPosition;
layout (location = 1) in vec2 v_RestXZ;

layout (location = 0) out vec4 color;

#include "Include/FrameSet.glslh"
#include "Include/WaterCommon.glslh"

void main()
{
	// Cyan in the troughs, yellow on the crests: the waves' height shows in the lines too, and both colors stand out from the
	// water's (dark blue up close, the bright sky reflected far away)
	float height = v_WorldPosition.y - u_DepthParams.w;
	float maxHeight = 0.0;
	for (int i = 0; i < min(int(u_GerstnerParams.x), MaxGerstnerWaves); i++)
	{
		maxHeight += u_GerstnerWaves[2 * i].w;
	}
	float crest = maxHeight > 0.0 ? clamp(0.5 + 0.5 * height / maxHeight, 0.0, 1.0) : 0.5;
	color = vec4(mix(vec3(0.0, 0.75, 1.0), vec3(1.0, 0.8, 0.1), crest), 1.0);
}
