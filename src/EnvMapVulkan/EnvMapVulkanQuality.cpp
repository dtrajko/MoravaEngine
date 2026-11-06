#include "EnvMapVulkanQuality.h"

#include <algorithm>


EnvMapVulkanRenderSettings EnvMapVulkanQuality::Apply(QualityTier tier, const EnvMapVulkanRenderSettings& scene)
{
	EnvMapVulkanRenderSettings settings = scene;
	switch (tier)
	{
		case QualityTier::Ultra:
			settings.Shadows.Resolution = std::min(scene.Shadows.Resolution * 2, 4096u);
			settings.LocalShadows.SpotResolution = std::min(scene.LocalShadows.SpotResolution * 2, 2048u);
			settings.LocalShadows.PointResolution = std::min(scene.LocalShadows.PointResolution * 2, 1024u);
			settings.Reflections.HalfResolution = false;
			settings.AmbientOcclusion.HalfResolution = false;
			settings.ProbeUpdateScale = 2.0f;
			break;
		case QualityTier::High:
		case QualityTier::Auto:
			break;
		case QualityTier::Medium:
			settings.RayTracing = false;
			settings.Shadows.Resolution = std::max(scene.Shadows.Resolution / 2, 1024u);
			settings.LocalShadows.SpotResolution = std::max(scene.LocalShadows.SpotResolution / 2, 512u);
			settings.LocalShadows.PointResolution = std::max(scene.LocalShadows.PointResolution / 2, 256u);
			settings.AmbientOcclusion.HalfResolution = true;
			settings.ProbeUpdateScale = 0.5f;
			break;
		case QualityTier::Low:
			settings.RayTracing = false;
			settings.Shadows.Resolution = 1024;
			settings.LocalShadows.SpotResolution = 512;
			settings.LocalShadows.PointResolution = 256;
			settings.AmbientOcclusion.Enabled = false;
			if (settings.Antialiasing == AntialiasingMode::SMAA || settings.Antialiasing == AntialiasingMode::TAA)
			{
				settings.Antialiasing = AntialiasingMode::FXAA;
			}
			settings.ProbeUpdateScale = 0.25f;
			break;
	}
	return settings;
}

const char* EnvMapVulkanQuality::Describe(QualityTier tier)
{
	switch (tier)
	{
		case QualityTier::Ultra:  return "larger shadow maps, full-resolution ray-traced effects";
		case QualityTier::High:   return "the scenes as they were made";
		case QualityTier::Medium: return "no ray tracing, smaller shadow maps";
		case QualityTier::Low:    return "no ray tracing, small shadow maps, no ambient occlusion, FXAA";
		case QualityTier::Auto:   return "the scenes as they were made";
	}
	return "";
}
