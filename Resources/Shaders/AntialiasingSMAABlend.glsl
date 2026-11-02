// SMAA, last pass (Vulkan, SceneEnvMapVulkan, 8.1 of docs/engine/Game_Engine_Runtime_Guide.html): each pixel mixed with its
// neighbors by the weights. Or, for the Environment panel's debug views, the edges or the weights themselves.
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
layout (binding = 1) uniform sampler2D u_WeightsTexture;
layout (binding = 2) uniform sampler2D u_EdgesTexture;

layout(push_constant) uniform Uniforms
{
	float View; // 0: the picture, 1: the edges, 2: the blend weights
} u_Uniforms;

#include "Include/SMAACommon.glslh"

void main()
{
	if (u_Uniforms.View > 1.5)
	{
		vec4 weights = textureLod(u_WeightsTexture, v_TexCoord, 0.0);
		o_Color = vec4(weights.rg, max(weights.b, weights.a), 1.0);
		return;
	}
	if (u_Uniforms.View > 0.5)
	{
		o_Color = vec4(textureLod(u_EdgesTexture, v_TexCoord, 0.0).rg, 0.0, 1.0);
		return;
	}

	SetMetrics(textureSize(u_ColorTexture, 0));
	vec4 offset;
	SMAANeighborhoodBlendingVS(v_TexCoord, offset);
	o_Color = vec4(SMAANeighborhoodBlendingPS(v_TexCoord, offset, u_ColorTexture, u_WeightsTexture).rgb, 1.0);
}
