// The water volume over the scene (SceneEnvMapVulkan, see EnvMapVulkanWater): a full-screen pass, drawn after the scene is
// copied and before the water surface. Every pixel's position is rebuilt from the copied depth; the part of the ray from
// the camera to it that is inside the water (under the surface, inside the water rectangle) absorbs its light and adds
// the water's own (Include/WaterCommon.glslh). That is the underwater fog when the camera is under the water, and also
// the water seen from the side of the rectangle, or across the waterline when the camera is half in the water.
// Where the water surface is drawn next, it replaces this (the surface accounts for the water in front of it itself).
// Descriptor sets: the water's (set 0 the per-frame set, set 1 the water's own), declared as in Water.glsl.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position; // a triangle covering the screen (clip space)

#include "Include/FrameCamera.glslh"
#include "Include/WaterSettings.glslh" // unused here: declared in both stages as in Water.glsl, so set 1's layouts are identical

void main()
{
	gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core

layout (location = 0) out vec4 color;

#include "Include/FrameSet.glslh"
#include "Include/WaterCommon.glslh"

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_SceneColor, 0));
	vec3 sceneColor = texture(u_SceneColor, uv).rgb;
	float depth = texture(u_SceneDepth, uv).r;
	if (u_ShadowDebug.x > 0.5)
	{
		color = vec4(sceneColor, 1.0); // Shadows Only: the shadow values as they are
		return;
	}

	// Back to the world: normalized device coordinates (Vulkan: y down, as the framebuffer rows) through the inverse view
	// projection; the sky (depth 1) lands on the far plane
	vec4 world = u_InverseViewProjection * vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec3 toPoint = world.xyz / world.w - u_CameraPosition;
	float distance = length(toPoint);

	color = vec4(distance > 0.0 ? ApplyWaterVolume(sceneColor, u_CameraPosition, toPoint / distance, distance) : sceneColor, 1.0);
}
