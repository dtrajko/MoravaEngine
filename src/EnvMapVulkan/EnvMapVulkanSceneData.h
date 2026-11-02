#pragma once

#include "EnvMapVulkanLights.h"
#include "EnvMapVulkanScene.h"

#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/ModelH2M.h"

#include <glm/glm.hpp>

#include <vector>


/**
 * The scene's components in the forms the renderer and the editor work with: the models and their meshes, the lights as
 * EnvMapVulkan*Light structs, the water and the probe volume with their placement taken from their transforms. Both read
 * the scene with these every frame (each into its own copies); the editor also writes edited lights back.
 *
 * Lights are entities (SunComponent, PointLightComponent, SpotLightComponent): their position is the transform's
 * translation and they shine along its -Y axis. A light read into its struct, edited and written back gets its direction
 * and position written only when they changed, so going through Euler angles can't make a light drift.
 */
namespace EnvMapVulkanSceneData
{
	// --- Models

	// The model entities, in the order of the hierarchy
	std::vector<EnvMapVulkanEntityID> GetModels(const EnvMapVulkanScene& scene);
	H2M::RefH2M<H2M::ModelH2M> GetModel(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID model);
	// The part entity of a model's mesh (NoEntity: none)
	EnvMapVulkanEntityID FindPart(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID model, int meshIndex);
	// The material each mesh of a model is drawn with (in the model's mesh order; the Default material for a mesh without a part)
	std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> GetMeshMaterials(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID model);
	// Where a mesh of a model with this transform is drawn (a rigged mesh of a skinned model: by its bones, from the root)
	glm::mat4 GetMeshTransform(const H2M::RefH2M<H2M::ModelH2M>& model, const H2M::RefH2M<H2M::MeshH2M>& mesh, const glm::mat4& transform);

	// --- Lights

	enum class LightKind { None, Sun, Point, Spot };

	// The lights' entities, in the order of EnvMapVulkanLightEnvironment's lists (the order of the hierarchy)
	struct LightEntities
	{
		EnvMapVulkanEntityID Sun = NoEntity;
		std::vector<EnvMapVulkanEntityID> PointLights;
		std::vector<EnvMapVulkanEntityID> SpotLights;

		// The entity of a light of the lists (NoEntity: none)
		EnvMapVulkanEntityID Get(LightKind kind, int index) const;
		// The light an entity is (None: it isn't one), and its index in its list
		LightKind Find(EnvMapVulkanEntityID entity, int& index) const;
	};

	// The scene's lights (a scene without a sun gets one that is off)
	LightEntities ReadLights(const EnvMapVulkanScene& scene, EnvMapVulkanLightEnvironment& lights);

	glm::vec3 GetEntityPosition(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity);
	void SetEntityPosition(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const glm::vec3& position);
	// The direction the entity's light travels (its world -Y axis)
	glm::vec3 GetEntityLightDirection(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity);
	void SetEntityLightDirection(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const glm::vec3& direction);

	EnvMapVulkanDirectionalLight ReadSun(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity);
	// before: the light as read (only what changed is written)
	void WriteSun(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const EnvMapVulkanDirectionalLight& sun, const EnvMapVulkanDirectionalLight& before);
	EnvMapVulkanPointLight ReadPointLight(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity);
	void WritePointLight(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const EnvMapVulkanPointLight& light, const EnvMapVulkanPointLight* before = nullptr);
	EnvMapVulkanSpotLight ReadSpotLight(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity);
	void WriteSpotLight(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const EnvMapVulkanSpotLight& light, const EnvMapVulkanSpotLight* before = nullptr);

	// --- The water and the probe volume (at most one of each)

	// The water's settings with its placement from the entity's transform (translation: center and height, rotation
	// around Y, scale: the size); Enabled when the scene has a water entity. Returns the entity (NoEntity: none).
	EnvMapVulkanEntityID ReadWater(const EnvMapVulkanScene& scene, EnvMapVulkanWaterSettings& settings);
	// The probe volume's settings with its placement from the entity's transform (translation: the box's center, scale:
	// its size); Exists when the scene has one. Returns the entity (NoEntity: none).
	EnvMapVulkanEntityID ReadProbeVolume(const EnvMapVulkanScene& scene, EnvMapVulkanProbeVolumeSettings& settings);
	glm::mat4 GetProbeVolumeTransform(const EnvMapVulkanProbeVolumeSettings& settings);

	// --- The cameras (CameraComponent, 6.3)

	// Puts a camera where the transform says: at its translation, looking as its rotation says (x: down (positive) and up,
	// as the editor's camera; y: around the vertical axis, 0 looking along -Z). The pitch is limited to -89..89 degrees,
	// as the camera's own.
	void PlaceCamera(H2M::CameraH2M& camera, const TransformComponent& transform);
	// Where a camera is and looks, as a transform PlaceCamera takes back (y within -180..180, no roll, scale 1)
	TransformComponent GetCameraPlacement(const H2M::CameraH2M& camera);

	// The camera the game is seen through: the first with Primary, else the first there is (NoEntity: none); cameras that
	// draw into a render texture (CameraComponent::Target) are left out
	EnvMapVulkanEntityID FindPrimaryCamera(const EnvMapVulkanScene& scene);
	// Where a camera entity is in the world and where it looks, as a transform PlaceCamera takes: a child (of a car, a
	// character) is where its parent puts it, turned with the parent around the vertical axis (a parent's tilt isn't
	// followed: a camera isn't rolled)
	TransformComponent GetGameCameraPlacement(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID cameraEntity);
	// The direction a camera's rotation looks (x down, y around the vertical axis, 0 along -Z), of length 1
	glm::vec3 GetCameraForward(const glm::vec3& rotation);
	// The camera placed as the game camera's entity is in the world, with its lens (field of view, near and far planes)
	void PlaceGameCamera(H2M::CameraH2M& camera, const EnvMapVulkanScene& scene, EnvMapVulkanEntityID cameraEntity);
}
