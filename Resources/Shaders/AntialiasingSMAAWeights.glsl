// SMAA, second pass (Vulkan, SceneEnvMapVulkan, 8.1 of docs/engine/Game_Engine_Runtime_Guide.html): how much of its
// neighbors each edge pixel takes, from the length and the shape of its edge (the area and search lookup textures).
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout (location = 0) out vec2 v_TexCoord;

void main()
{
	// From the position, not a_TexCoord: pixel for pixel, each pass leaves the picture as it is
	v_TexCoord = a_Position.xy * 0.5 + 0.5;
	gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout (location = 0) in vec2 v_TexCoord;

layout (binding = 0) uniform sampler2D u_EdgesTexture;
layout (binding = 1) uniform sampler2D u_AreaTexture;
layout (binding = 2) uniform sampler2D u_SearchTexture;

#include "Include/SMAACommon.glslh"

void main()
{
	SetMetrics(textureSize(u_EdgesTexture, 0));
	vec2 pixcoord;
	vec4 offset[3];
	SMAABlendingWeightCalculationVS(v_TexCoord, pixcoord, offset);
	o_Color = SMAABlendingWeightCalculationPS(v_TexCoord, pixcoord, offset, u_EdgesTexture, u_AreaTexture, u_SearchTexture, vec4(0.0));
}
