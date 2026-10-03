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

	glm::vec3 GetDirection() const { return DirectionFromAngles(Azimuth, Elevation); }
	void SetDirection(const glm::vec3& towardsLight) { AnglesFromDirection(towardsLight, Azimuth, Elevation); }
};

struct EnvMapVulkanPointLight
{
	std::string Name;
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 10.0f;
	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 10.0f; // the light reaches exactly zero here
};

struct EnvMapVulkanSpotLight
{
	std::string Name;
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 30.0f; // a spot is added 3 units above a surface (a point light 1.5): about the same light arrives
	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 10.0f;
	// Where the spot points (the direction the light travels); the default points straight down
	float Azimuth = 0.0f;
	float Elevation = -90.0f;
	// Half-angles of the cone (degrees): full intensity inside InnerAngle, fading to zero at OuterAngle
	float InnerAngle = 20.0f;
	float OuterAngle = 30.0f;

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
		float Padding[3];    // three floats, not a vec3: a std140 vec3 would be aligned to 16 bytes
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
	static_assert(sizeof(PointLight) == 32, "std140 layout mismatch");
	static_assert(sizeof(SpotLight) == 64, "std140 layout mismatch");
	static_assert(sizeof(LightsUB) == 48 + 32 * MaxPointLights + 64 * MaxSpotLights, "std140 layout mismatch");
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

	// A new light with a unique name ("Point Light 1"...), or nullptr when the list is full.
	// The pointer is valid until the list changes.
	EnvMapVulkanPointLight* AddPointLight(const glm::vec3& position);
	EnvMapVulkanSpotLight* AddSpotLight(const glm::vec3& position);

	// Fills the uniform buffer: directions normalized, cone angles as cosines, values clamped to valid ranges
	void Pack(EnvMapVulkanLightsGPU::LightsUB& out) const;

private:
	uint32_t m_NextPointLightNumber = 1;
	uint32_t m_NextSpotLightNumber = 1;
};
