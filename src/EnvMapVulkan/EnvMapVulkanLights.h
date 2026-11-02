#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


/**
 * Lights of SceneEnvMapVulkan: one directional light (the sun) plus lists of point and spot lights.
 * Intensity is an artistic unit: the light's radiance is Color * Intensity, and point and spot lights fall off with the
 * inverse square of the distance, windowed to reach exactly zero at Range.
 * Directions are edited as angles in degrees: azimuth around +Y (0 = +Z, 90 = +X) and elevation above the horizon.
 */

// Unit vector for an azimuth/elevation pair (degrees), and back
glm::vec3 DirectionFromAngles(float azimuth, float elevation);
void AnglesFromDirection(const glm::vec3& direction, float& azimuth, float& elevation);

/**
 * The brightest light source of an equirectangular HDR environment map (RGBA32F, rows from the top), used by
 * "Align to Environment": the brightest region above the horizon (a block of about 17 x 17 degrees with the most light),
 * then the luminance-weighted center of its pixels at least half as bright as its peak: the center of the sun disc, or
 * of a bright window in a map without a sun. Below the horizon are only reflections (a sunlit floor), never the sun.
 * direction: in the map's own space (EquirectangularToCubeMap.glsl), before the environment rotation.
 * color: the average color of those pixels, scaled so the brightest channel is 1.
 * Returns false for an empty or uniformly black map.
 */
bool FindBrightestDirection(const float* rgba, uint32_t width, uint32_t height, glm::vec3& direction, glm::vec3& color);

// A sun taken out of an environment map (ExtractSun), as a directional light
struct EnvMapVulkanExtractedSun
{
	bool Found = false;
	glm::vec3 Direction = glm::vec3(0.0f, 1.0f, 0.0f); // towards the sun, in the map's own space (before the environment rotation)
	glm::vec3 Color = glm::vec3(1.0f);                 // brightest channel 1
	float Intensity = 0.0f;                            // the removed light, as EnvMapVulkanDirectionalLight::Intensity
	float PeakToAverage = 0.0f;                        // how much brighter than the map's average the sun's brightest pixel was
	uint32_t PixelCount = 0;                           // pixels dimmed
};

/**
 * Takes a real sun out of an equirectangular HDR map (RGBA32F, rows from the top), in place, before the environment
 * lighting is built from the map, and returns its light as a directional light. The sun then casts shadows with all of
 * its light, and isn't counted twice (once in the environment lighting, where nothing can shadow it, and once as the
 * directional light).
 * - A sun: a compact source above the horizon, at least 1000x brighter than the map's average. A bright window or sky
 *   is not one (newport_loft.hdr has none; rooitou_park_4k.hdr's sun is 80,000x brighter than the average).
 * - Removed: the pixels within 5 degrees of the brightest one (the disc and its glow) that are brighter than 50x the
 *   average. They are dimmed to that level, so the skybox still shows a bright sun; the light above it goes to the
 *   directional light, intensity = irradiance / pi (EnvironmentIrradiance.glsl bakes in a white Lambertian surface).
 * Returns false (and leaves the pixels as they are) when the map has no sun.
 */
bool ExtractSun(float* rgba, uint32_t width, uint32_t height, EnvMapVulkanExtractedSun& sun);

struct EnvMapVulkanDirectionalLight
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	// Where the sun is in the sky (the direction from a surface towards the light). The default is the old
	// hard-coded light direction (0.5, 0.5, 0.5).
	float Azimuth = 45.0f;
	float Elevation = 35.2644f;
	// Turns with the environment map: rotating the skybox by N degrees adds N degrees to the azimuth, so a sun aligned
	// to the HDR map stays on the sun of the map
	bool FollowEnvironmentRotation = true;
	bool CastShadows = true; // cascaded shadow maps, see EnvMapVulkanShadows.h
	// Editor only (a directional light has no position; this doesn't change the light): the icon is drawn in the sky until
	// it is moved with the gizmo, then at IconPosition
	bool IconMoved = false;
	glm::vec3 IconPosition = glm::vec3(0.0f);

	glm::vec3 GetDirection() const { return DirectionFromAngles(Azimuth, Elevation); }
	void SetDirection(const glm::vec3& towardsLight) { AnglesFromDirection(towardsLight, Azimuth, Elevation); }
};

struct EnvMapVulkanPointLight
{
	std::string Name;
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 3.0f; // added 1.5 units above a surface: about 1.3 arrives there (a brighter light saturates a white surface)
	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 10.0f; // the light reaches exactly zero here
	bool CastShadows = false; // see MaxShadowedPointLights (EnvMapVulkanShadows.h)
};

struct EnvMapVulkanSpotLight
{
	std::string Name;
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 10.0f; // added 3 units above a surface (a point light 1.5): about the same light arrives, 1.1
	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 10.0f;
	// Where the spot points (the direction the light travels); the default points straight down
	float Azimuth = 0.0f;
	float Elevation = -90.0f;
	// Half-angles of the cone (degrees): full intensity inside InnerAngle, fading to zero at OuterAngle
	float InnerAngle = 20.0f;
	float OuterAngle = 30.0f;
	bool CastShadows = false; // see MaxShadowedSpotLights (EnvMapVulkanShadows.h)

	glm::vec3 GetDirection() const { return DirectionFromAngles(Azimuth, Elevation); }
	void SetDirection(const glm::vec3& direction) { AnglesFromDirection(direction, Azimuth, Elevation); }
};

/**
 * The Lights uniform buffer of the mesh shaders (set 0, binding 5, std140), see HazelPBR_Static.glsl / HazelPBR_Anim.glsl.
 * The layouts must match the shaders byte for byte.
 */
namespace EnvMapVulkanLightsGPU
{
	constexpr uint32_t MaxPointLights = 16;
	constexpr uint32_t MaxSpotLights = 16;

	struct DirectionalLight
	{
		glm::vec3 Direction; // towards the light
		float Intensity;     // 0 when the sun is disabled
		glm::vec3 Color;
		float Padding;
	};

	struct PointLight
	{
		glm::vec3 Position;
		float Intensity;
		glm::vec3 Color;
		float Range;
		float ShadowIndex;   // the light's cube in the point light shadow maps, -1: no shadows
		float Padding[3];
	};

	struct SpotLight
	{
		glm::vec3 Position;
		float Intensity;
		glm::vec3 Color;
		float Range;
		glm::vec3 Direction; // the direction the light travels
		float CosOuter;
		float CosInner;
		float ShadowIndex;   // the light's layer in the spot light shadow maps, -1: no shadows
		float Padding[2];    // floats, not a vector: a std140 vec2 / vec3 would be aligned (and move the fields)
	};

	struct LightsUB
	{
		DirectionalLight Sun;
		int32_t PointLightCount;
		int32_t SpotLightCount;
		int32_t Padding[2];
		PointLight PointLights[MaxPointLights];
		SpotLight SpotLights[MaxSpotLights];
	};

	static_assert(sizeof(DirectionalLight) == 32, "std140 layout mismatch");
	static_assert(sizeof(PointLight) == 48, "std140 layout mismatch");
	static_assert(sizeof(SpotLight) == 64, "std140 layout mismatch");
	static_assert(sizeof(LightsUB) == 48 + 48 * MaxPointLights + 64 * MaxSpotLights, "std140 layout mismatch");
}

/**
 * All lights of the scene. The lists hold disabled lights too (they count towards the limits); Pack leaves them out,
 * so the shaders only loop over lights that shine.
 */
class EnvMapVulkanLightEnvironment
{
public:
	EnvMapVulkanDirectionalLight Sun;
	std::vector<EnvMapVulkanPointLight> PointLights;
	std::vector<EnvMapVulkanSpotLight> SpotLights;

	bool CanAddPointLight() const { return PointLights.size() < EnvMapVulkanLightsGPU::MaxPointLights; }
	bool CanAddSpotLight() const { return SpotLights.size() < EnvMapVulkanLightsGPU::MaxSpotLights; }

	// A new light with a unique name ("Point Light 1"...), or nullptr when the list is full. It casts shadows when fewer
	// than maxShadowed lights of its kind do. The pointer is valid until the list changes.
	EnvMapVulkanPointLight* AddPointLight(const glm::vec3& position, uint32_t maxShadowed);
	EnvMapVulkanSpotLight* AddSpotLight(const glm::vec3& position, uint32_t maxShadowed);

	uint32_t CountShadowedPointLights() const;
	uint32_t CountShadowedSpotLights() const;

	// Fills the uniform buffer: directions normalized, cone angles as cosines, values clamped to valid ranges.
	// pointShadowSlots / spotShadowSlots: per light (same order as the lists), its shadow slot this frame or -1
	// (empty: no shadows)
	void Pack(EnvMapVulkanLightsGPU::LightsUB& out, const std::vector<int>& pointShadowSlots = {}, const std::vector<int>& spotShadowSlots = {}) const;

private:
	uint32_t m_NextPointLightNumber = 1;
	uint32_t m_NextSpotLightNumber = 1;
};
