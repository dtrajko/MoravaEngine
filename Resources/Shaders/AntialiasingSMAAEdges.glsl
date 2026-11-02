// SMAA, first pass (Vulkan, SceneEnvMapVulkan, 8.1 of docs/engine/Game_Engine_Runtime_Guide.html): where the finished
// picture has edges, from the color differences to the left and top neighbors. r: an edge at the left, g: at the top.
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

layout (binding = 0) uniform sampler2D u_ColorTexture; // the composite: gamma encoded

#include "Include/SMAACommon.glslh"

void main()
{
	SetMetrics(textureSize(u_ColorTexture, 0));
	vec4 offset[3];
	SMAAEdgeDetectionVS(v_TexCoord, offset);
	vec2 edges = SMAAColorEdgeDetectionPS(v_TexCoord, offset, u_ColorTexture); // discards a pixel without edges
	o_Color = vec4(edges, 0.0, 1.0);
}
