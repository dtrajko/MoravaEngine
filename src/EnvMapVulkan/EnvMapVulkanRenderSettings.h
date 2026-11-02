#pragma once

#include "EnvMapVulkanReflections.h"
#include "EnvMapVulkanShadows.h"
#include "EnvMapVulkanSSAO.h"

#include <yaml-cpp/yaml.h>


// Bloom: the bright parts of the scene are downsampled into a chain of half-resolution levels and upsampled back, which
// spreads them into a wide glow that the viewport composite adds to the scene.
struct EnvMapVulkanBloomSettings
{
	bool Enabled = true;
	float Threshold = 1.0f;     // brightness (after exposure) where bloom starts
	float Knee = 0.1f;          // soft transition below the threshold
	float UpsampleScale = 1.0f; // radius of the upsample filter: larger values spread the glow further
	float Intensity = 1.0f;
	bool DirtEnabled = false;   // lens dirt on/off (the texture and intensity are kept)
	float DirtIntensity = 1.0f; // lens dirt texture, lit by the bloom
};

/**
 * How the renderer draws a scene: the settings saved with it (the RenderSettings map of an .mscene file). The editor's
 * panels change them, the renderer only reads them.
 *
 * The shadow map resolutions are the sizes asked for: the renderer recreates a map at the start of the next frame when
 * its size differs (its descriptor can't change while a recorded frame uses it).
 */
struct EnvMapVulkanRenderSettings
{
	EnvMapVulkanShadowSettings Shadows;           // the sun's
	EnvMapVulkanLocalShadowSettings LocalShadows; // the spot and point lights'
	bool RayTracing = true;                       // where the GPU can: off, everything falls back to its raster way
	EnvMapVulkanReflectionSettings Reflections;
	EnvMapVulkanSSAOSettings AmbientOcclusion;
	EnvMapVulkanBloomSettings Bloom;

	// The content of the RenderSettings map
	void Serialize(YAML::Emitter& out) const;
	// From the RenderSettings map; a section the map doesn't have (an older file) keeps its defaults
	static EnvMapVulkanRenderSettings Deserialize(const YAML::Node& node);
};
