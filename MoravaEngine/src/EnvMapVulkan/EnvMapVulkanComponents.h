#pragma once

#include "EnvMapVulkanProbes.h"
#include "EnvMapVulkanWater.h"

#include "H2M/Core/RefH2M.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


namespace H2M
{
	class ModelH2M;
}
class EnvMapVulkanMaterial;

/**
 * The components of the SceneEnvMapVulkan scene (EnvMapVulkanScene): plain data, no logic beyond small helpers. Every
 * entity has an ID, a name, a place in the hierarchy and a transform; the other components say what it is.
 *
 * Conventions:
 * - Entities refer to each other by ID (EnvMapVulkanEntityID), never by registry handle or list index, so references
 *   survive removal, reordering and saving.
 * - Rotations are Euler angles in degrees, in ImGuizmo's convention (RecomposeMatrixFromComponents), so the values in
 *   the panels and the gizmo round-trip exactly.
 * - Lights shine along their transform's local -Y axis: with no rotation a spot light points straight down and the sun
 *   is overhead. A light's position is its transform's translation (for the sun, where its icon is drawn).
 */

// An entity's ID: unique, random, stable for the entity's life and across saving and loading. 0 is "no entity".
using EnvMapVulkanEntityID = uint64_t;
constexpr EnvMapVulkanEntityID NoEntity = 0;

struct IDComponent
{
	EnvMapVulkanEntityID ID = NoEntity;
};

struct NameComponent
{
	std::string Name;
};

// The entity's place in the tree. Children are kept in order (the order the hierarchy panel shows them in).
struct HierarchyComponent
{
	EnvMapVulkanEntityID Parent = NoEntity;
	std::vector<EnvMapVulkanEntityID> Children;
};

// Relative to the parent (to the world for an entity without a parent)
struct TransformComponent
{
	glm::vec3 Translation = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f); // degrees (ImGuizmo's Euler convention)
	glm::vec3 Scale = glm::vec3(1.0f);

	glm::mat4 GetMatrix() const;
	void SetMatrix(const glm::mat4& matrix); // decomposed into translation, rotation and scale

	// The direction a light with this transform shines (its local -Y axis, normalized), and the rotation that turns it
	// toward a direction (no roll around it)
	glm::vec3 GetLightDirection() const;
	void SetLightDirection(const glm::vec3& direction);
};

// A model loaded from a file. Its parts (the meshes of the file) are child entities with a MeshPartComponent.
struct ModelComponent
{
	std::string FilePath;
	H2M::RefH2M<H2M::ModelH2M> Model; // the loaded model (each model entity has its own, so its parts can be edited)
};

// A part of a model: one of its meshes. Its TransformComponent is the part's place in the model (relative to the model
// entity, its parent).
struct MeshPartComponent
{
	uint32_t MeshIndex = 0;                            // in the model's meshes (shifts when other parts are removed)
	uint32_t SourceMeshIndex = 0;                      // in the model file's meshes (stable: saved scenes match parts by it)
	H2M::RefH2M<EnvMapVulkanMaterial> Material;        // from the Material Library (by ID in saved scenes, see Phase 4)
	glm::mat4 OriginalTransform = glm::mat4(1.0f);     // as loaded from the file (Reset Mesh)
	// Runtime: the transform last written into the model's mesh (the part's transform is written again only when it
	// differs, so an imported transform Euler angles can't reproduce exactly, e.g. a mirrored part, stays until edited)
	TransformComponent AppliedTransform;
};

// The sun: a directional light. It shines along its transform's -Y axis; the translation only places its icon.
struct SunComponent
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	bool FollowEnvironmentRotation = true; // turns with the environment map (a sun aligned to the HDR stays on its sun)
	bool CastShadows = true;
	bool IconPlaced = false;               // false: the icon is drawn in the sky, along the light; true: at the translation
};

struct PointLightComponent
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 3.0f;
	float Range = 10.0f; // the light reaches exactly zero here
	bool CastShadows = false;
};

// Shines along its transform's -Y axis
struct SpotLightComponent
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 10.0f;
	float Range = 10.0f;
	float InnerAngle = 20.0f; // half-angles of the cone (degrees): full intensity inside, fading to zero at OuterAngle
	float OuterAngle = 30.0f;
	bool CastShadows = false;
};

// The water plane (at most one per scene). Its placement is the entity's transform: translation = center and height,
// rotation around Y only (the other two are kept at 0), scale = size in meters (Y kept at 1). The settings' own Center,
// Height, Size and Rotation are filled from the transform before rendering.
struct WaterComponent
{
	EnvMapVulkanWaterSettings Settings;
};

// The probe volume (at most one per scene): a box with a grid of probes that hold the scene's indirect light (see
// EnvMapVulkanProbes). Its placement is the entity's transform: translation = the box's center, scale = its size in
// meters; it isn't turned (the rotation is kept at 0). The settings' own Center and Size are filled from the transform
// before rendering.
struct ProbeVolumeComponent
{
	EnvMapVulkanProbeVolumeSettings Settings;
};

// The environment: the HDR map the scene is lit by and its skybox (one per scene)
struct EnvironmentComponent
{
	std::string FilePath;            // the .hdr map
	float Rotation = 0.0f;           // degrees around Y
	float Exposure = 1.0f;
	bool AutoExposure = true;
	float HuePreservation = 0.5f;    // tonemapping: 0 ACES per channel, 1 hue-preserving
	bool ExtractSun = true;          // a real sun in the map is taken out of the lighting and given to the sun entity
	float SkyboxLod = 0.0f;
};
