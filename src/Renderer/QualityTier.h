#pragma once

#include <string>


/**
 * How much of the renderer's work a game asks of the computer (docs/engine/Game_Engine_Runtime_Guide.html, 11.4). High is
 * the scene as it was made in the editor. Ultra asks more of it, Medium and Low less: what each tier does to the scene's
 * render settings is EnvMapVulkanQuality::Apply. Auto is no tier: the game picks one for the GPU (ChooseQualityTier).
 *
 * The numbers are C#'s QualityLevel.
 */
enum class QualityTier : int
{
	Low = 0,
	Medium = 1,
	High = 2,
	Ultra = 3,
	Auto = -1,
};

inline const char* GetQualityTierName(QualityTier tier)
{
	switch (tier)
	{
		case QualityTier::Low:    return "Low";
		case QualityTier::Medium: return "Medium";
		case QualityTier::High:   return "High";
		case QualityTier::Ultra:  return "Ultra";
		case QualityTier::Auto:   return "Auto";
	}
	return "Auto";
}

// False when the name is none of the tiers (Auto is one)
inline bool FindQualityTier(const std::string& name, QualityTier& tier)
{
	for (QualityTier candidate : { QualityTier::Low, QualityTier::Medium, QualityTier::High, QualityTier::Ultra, QualityTier::Auto })
	{
		if (name == GetQualityTierName(candidate))
		{
			tier = candidate;
			return true;
		}
	}
	return false;
}

// What the GPU can do, for Auto
struct QualityGPU
{
	bool RayTracing = false; // the renderer traces rays on it (not turned off with --no-ray-tracing)
	bool Discrete = false;   // a graphics card of its own, not one in the processor
};

// Auto's tier: High on a graphics card with ray tracing, Medium on one without it, Low on a GPU in the processor (with ray
// tracing or not: it shares the processor's memory and power). Ultra is never picked: it is the player's choice
inline QualityTier ChooseQualityTier(const QualityGPU& gpu)
{
	if (!gpu.Discrete)
	{
		return QualityTier::Low;
	}
	return gpu.RayTracing ? QualityTier::High : QualityTier::Medium;
}
