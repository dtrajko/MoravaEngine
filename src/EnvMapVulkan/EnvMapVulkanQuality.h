#pragma once

#include "EnvMapVulkanRenderSettings.h"

#include "Renderer/QualityTier.h"


/**
 * The quality tiers (docs/engine/Game_Engine_Runtime_Guide.html, 11.4): a scene's render settings as a tier draws them. The
 * scene keeps its own; the renderer applies the tier to its copy each frame (EnvMapVulkanRenderer::SetQualityTier).
 *
 *   Ultra   sun shadow maps twice as large (at most 4096), the local lights' too, ray-traced reflections and ambient
 *           occlusion at full resolution, the probes updated twice as fast
 *   High    the scene as it was made (the editor shows it so)
 *   Medium  no ray tracing (screen-space ambient occlusion, the environment's reflections, probes captured by drawing),
 *           shadow maps half as large (at least 1024, 512, 256), the probes updated half as fast
 *   Low     as Medium, with the smallest shadow maps, no ambient occlusion, FXAA for SMAA and TAA, the probes updated at a
 *           quarter of the speed
 *
 * A GPU without ray tracing draws High and Ultra without it, as it draws every scene. The render scale the guide planned is
 * not here: the renderer draws everything at the window's size.
 */
namespace EnvMapVulkanQuality
{
	// The settings the tier draws a scene's settings with (Auto: as High)
	EnvMapVulkanRenderSettings Apply(QualityTier tier, const EnvMapVulkanRenderSettings& scene);

	// What the tier changes, in a few words (the log)
	const char* Describe(QualityTier tier);
}
