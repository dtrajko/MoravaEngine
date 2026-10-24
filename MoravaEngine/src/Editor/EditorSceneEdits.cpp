// Changes to the scene that the panels and the viewport share: the environment, the sun and lights, the water, the probe volume and the camera entity; models loaded; entities added and deleted; what kind an entity is


#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanProfiler.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Core/Application.h"
#include "Core/FileDialogs.h"
#include "Core/Input.h"
#include "Core/ResourceManager.h"
#include "Core/Util.h"
#include "H2M/Core/Math/RayH2M.h"
#include "ImGui/ImGuiWrapper.h"
#include "Scene/Scene.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"

#include "imgui.h"
#include "imgui_internal.h" // BeginDragDropTargetCustom (the Models and Meshes panel as one drop area)
#include "ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>


#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

// The model of the selection (the selected model, or the model of the selected part); NoEntity when no model is selected
EnvMapVulkanEntityID GetSelectedModel()
{
	if (s_Scene.Has<ModelComponent>(s_EditorSelection.GetPrimary()))
	{
		return s_EditorSelection.GetPrimary();
	}
	if (s_Scene.Has<MeshPartComponent>(s_EditorSelection.GetPrimary()))
	{
		return s_Scene.GetParent(s_EditorSelection.GetPrimary());
	}
	return NoEntity;
}

// The selected part's mesh index in its model; -1 when no part is selected
int GetSelectedMeshIndex()
{
	const MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(s_EditorSelection.GetPrimary());
	return part ? (int)part->MeshIndex : -1;
}

// The part of a model's mesh, or the model itself (meshIndex < 0, or no part for that mesh)
EnvMapVulkanEntityID GetModelOrPart(EnvMapVulkanEntityID model, int meshIndex)
{
	EnvMapVulkanEntityID part = meshIndex >= 0 ? FindPart(s_Scene, model, meshIndex) : NoEntity;
	return part != NoEntity ? part : model;
}

// Selects a model, or one of its parts (meshIndex >= 0), alone
static void SelectModel(EnvMapVulkanEntityID model, int meshIndex = -1)
{
	s_EditorSelection.SelectOnly(GetModelOrPart(model, meshIndex));
}

// A map chosen in the editor (Load button, a file dropped): the environment entity asks for it
void RequestEnvironmentMap(const std::string& filepath)
{
	if (EnvironmentComponent* environment = s_Scene.TryGet<EnvironmentComponent>(s_EnvironmentEntity))
	{
		environment->FilePath = filepath;
	}
}

// The scene's environment entity: the default settings, with the map loaded now
void CreateEnvironmentEntity()
{
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Environment");
	s_Scene.Add<EnvironmentComponent>(entity).FilePath = EnvMapVulkanRenderer::GetEnvironmentMapFile();
	s_EnvironmentEntity = entity;
}

// The camera is an entity with a CameraComponent (one per scene, as the environment and the sun): its transform is where
// the editor's camera is and where it looks. SyncCamera keeps the two the same, every frame: flying the camera in the
// viewport moves the entity, and a transform that changed without the camera (edited in the Inspector, or an opened
// scene's) moves the camera.
static EnvMapVulkanEntityID s_CameraEntity = NoEntity;
static glm::vec3 s_SyncedCameraRotation = glm::vec3(0.0f);

// A scene without a camera entity gets one (a new scene, a scene saved before cameras were entities): where the
// editor's camera is now, written by the next SyncCamera
void EnsureCameraEntity()
{
	s_CameraEntity = s_Scene.FindFirst<CameraComponent>();
	if (s_CameraEntity == NoEntity)
	{
		s_CameraEntity = s_Scene.CreateEntity("Camera");
		s_Scene.Add<CameraComponent>(s_CameraEntity);
		s_CameraEntitySynced = false;
	}
	// The first entity of the Scene Hierarchy
	if (s_Scene.GetRoots().empty() || s_Scene.GetRoots().front() != s_CameraEntity)
	{
		s_Scene.SetParent(s_CameraEntity, NoEntity, true, 0);
	}
}

void SyncCamera(H2M::CameraH2M* camera)
{
	EnsureCameraEntity();
	TransformComponent& transform = s_Scene.Get<TransformComponent>(s_CameraEntity);
	if (s_CameraEntitySynced && (transform.Translation != s_SyncedCameraTranslation || transform.Rotation != s_SyncedCameraRotation))
	{
		// The entity moved: the camera follows (now: the camera's controller runs before the next frame is drawn, not this one)
		transform.Rotation.x = glm::clamp(transform.Rotation.x, -89.0f, 89.0f);
		transform.Rotation.z = 0.0f;
		PlaceCamera(*camera, transform);
	}
	else
	{
		// The turn around the vertical axis, kept within -180 to 180
		float turn = std::fmod(-camera->GetYaw() - 90.0f, 360.0f);
		turn += turn > 180.0f ? -360.0f : (turn <= -180.0f ? 360.0f : 0.0f);
		transform.Translation = camera->GetPosition();
		transform.Rotation = glm::vec3(camera->GetPitch(), turn, 0.0f);
	}
	transform.Scale = glm::vec3(1.0f);
	// The lens: the projection is rebuilt from it with the viewport's size (OnUpdate)
	CameraComponent& component = s_Scene.Get<CameraComponent>(s_CameraEntity);
	component.FieldOfView = glm::clamp(component.FieldOfView, CameraComponent::MinFieldOfView, CameraComponent::MaxFieldOfView);
	camera->SetPerspectiveVerticalFOV(component.FieldOfView);
	s_CameraEntitySynced = true;
	s_SyncedCameraTranslation = transform.Translation;
	s_SyncedCameraRotation = transform.Rotation;
}

// Sets a part's transform to an exact matrix (the gizmo's result, Reset Mesh): the mesh gets the matrix as it is, the
// part's transform its decomposition
void SetPartMatrix(EnvMapVulkanEntityID partEntity, const glm::mat4& matrix)
{
	MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(partEntity);
	H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, s_Scene.GetParent(partEntity));
	if (!part || !model || part->MeshIndex >= model->GetMeshes().size())
	{
		return;
	}
	TransformComponent& transform = s_Scene.Get<TransformComponent>(partEntity);
	transform.SetMatrix(matrix);
	part->AppliedTransform = transform;
	model->GetMeshes()[part->MeshIndex]->Transform = matrix;
}

// Loads a model file and adds it to the scene (called at the start of a frame, see EditorPreFrame), and selects it. With a ground position (a model dropped
// on the viewport), the model stands on that point: the bottom center of its bounding box is placed there, rather than its origin.
// A model far too large or too small for the view (e.g. modeled in millimeters) is also scaled to fit (see below).
void LoadModel(const std::string& filepath, std::optional<glm::vec3> groundPosition)
{
	const EnvMapVulkanEntityID entity = EnvMapVulkanSceneLoader::AddModel(s_Scene, filepath);
	if (entity == NoEntity)
	{
		return;
	}
	H2M::RefH2M<H2M::ModelH2M> model = s_Scene.Get<ModelComponent>(entity).Model;
	TransformComponent& entry = s_Scene.Get<TransformComponent>(entity);

	if (groundPosition)
	{
		// Bounding box of the model at rest: the corners of every mesh's bounding box, placed as they are drawn
		glm::vec3 boundsMin(std::numeric_limits<float>::max());
		glm::vec3 boundsMax(-std::numeric_limits<float>::max());
		for (auto& mesh : model->GetMeshes())
		{
			glm::mat4 meshTransform = GetMeshTransform(model, mesh, glm::mat4(1.0f));
			const H2M::AABB_H2M& box = mesh->BoundingBox;
			for (int corner = 0; corner < 8; corner++)
			{
				glm::vec3 point((corner & 1) ? box.Max.x : box.Min.x, (corner & 2) ? box.Max.y : box.Min.y, (corner & 4) ? box.Max.z : box.Min.z);
				glm::vec3 placed = glm::vec3(meshTransform * glm::vec4(point, 1.0f));
				boundsMin = glm::min(boundsMin, placed);
				boundsMax = glm::max(boundsMax, placed);
			}
		}

		if (boundsMin.x <= boundsMax.x)
		{
			// Fit to the view, only for extreme cases: a model that can't fit in the view at all (larger than 10x the distance from the
			// camera to the drop point, e.g. modeled in millimeters) or would be a few pixels (smaller than 1/200 of that distance)
			// gets a uniform scale that makes it a quarter of the distance. Every other model keeps its scale of 1.
			float size = glm::compMax(boundsMax - boundsMin);
			glm::vec3 cameraPosition = glm::vec3(glm::inverse((*s_Camera).GetViewMatrix())[3]);
			float distance = glm::max(glm::length(*groundPosition - cameraPosition), 1.0f);
			if (size > 0.0f && (size > distance * 10.0f || size < distance * 0.005f))
			{
				entry.Scale = glm::vec3(distance * 0.25f / size);
				Log::GetLogger()->info("Model '{0}' is {1} units across: scaled by {2} to fit the view (set Scale to 1 for its original size)",
					filepath, size, entry.Scale.x);
			}

			// The bottom center of the (scaled) bounding box goes to the drop point
			glm::vec3 bottomCenter((boundsMin.x + boundsMax.x) * 0.5f, boundsMin.y, (boundsMin.z + boundsMax.z) * 0.5f);
			entry.Translation = *groundPosition - bottomCenter * entry.Scale;
		}
		else
		{
			entry.Translation = *groundPosition;
		}
	}

	s_EditorSelection.SetPrimary(entity);
}

// Writes an edited copy of a light of s_Lights back into its entity (before: the light as it was read)
void CommitPointLight(int index, const EnvMapVulkanPointLight& before)
{
	EnvMapVulkanEntityID entity = s_LightEntities.Get(LightKind::Point, index);
	if (entity != NoEntity)
	{
		WritePointLight(s_Scene, entity, s_Lights.PointLights[index], &before);
	}
}

void CommitSpotLight(int index, const EnvMapVulkanSpotLight& before)
{
	EnvMapVulkanEntityID entity = s_LightEntities.Get(LightKind::Spot, index);
	if (entity != NoEntity)
	{
		WriteSpotLight(s_Scene, entity, s_Lights.SpotLights[index], &before);
	}
}

void CommitSun(const EnvMapVulkanDirectionalLight& before)
{
	WriteSun(s_Scene, s_LightEntities.Sun, s_Lights.Sun, before);
}

// A new point or spot light entity from a light struct (EnvMapVulkanLightEnvironment::Add*Light makes one with its defaults)
EnvMapVulkanEntityID CreatePointLightEntity(const EnvMapVulkanPointLight& light)
{
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity(light.Name);
	s_Scene.Add<PointLightComponent>(entity);
	WritePointLight(s_Scene, entity, light);
	return entity;
}

EnvMapVulkanEntityID CreateSpotLightEntity(const EnvMapVulkanSpotLight& light)
{
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity(light.Name);
	s_Scene.Add<SpotLightComponent>(entity);
	WriteSpotLight(s_Scene, entity, light);
	return entity;
}

// The scene's sun (one per scene), with the defaults of EnvMapVulkanDirectionalLight
void CreateSunEntity()
{
	EnvMapVulkanDirectionalLight defaults;
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Sun");
	s_Scene.Add<SunComponent>(entity);
	EnvMapVulkanDirectionalLight none = defaults;
	none.Azimuth = none.Elevation = 1000.0f; // differs from the defaults: the direction is written
	WriteSun(s_Scene, entity, defaults, none);
}

// The sun turns with the environment map (EnvMapVulkanDirectionalLight::FollowEnvironmentRotation). Checked once per frame,
// so every way of changing the rotation (Environment panel, code) is covered. The shaders look the environment up at
// RotateVectorAboutY(rotation, worldDirection), so a direction of the map has the azimuth (map azimuth + rotation) in
// the world: a rotation change of N degrees moves the sun by N degrees of azimuth. A scene opened or created starts over
// from its own rotation (see ForgetSceneEntities): only changes made while it is open turn its sun.
static float s_SunSyncedEnvMapRotation = 0.0f;

void SyncSunWithEnvironmentRotation()
{
	if (!s_SunRotationSynced)
	{
		s_SunSyncedEnvMapRotation = GetEnvironmentRotation();
		s_SunRotationSynced = true;
		return;
	}
	float delta = GetEnvironmentRotation() - s_SunSyncedEnvMapRotation;
	s_SunSyncedEnvMapRotation = GetEnvironmentRotation();
	if (delta == 0.0f || !s_Lights.Sun.FollowEnvironmentRotation)
	{
		return;
	}
	// The rotation wraps around at 360 degrees: 359 -> 1 is a change of +2, not -358
	delta = std::remainder(delta, 360.0f);
	const EnvMapVulkanDirectionalLight before = s_Lights.Sun;
	s_Lights.Sun.Azimuth = std::remainder(s_Lights.Sun.Azimuth + delta, 360.0f); // stays in -180..180
	CommitSun(before);
}

// Writes an edited s_WaterSettings back into the water entity (before: as it was read); the placement only when it changed
void CommitWater(const EnvMapVulkanWaterSettings& before)
{
	if (s_WaterEntity == NoEntity || !s_Scene.Exists(s_WaterEntity))
	{
		return;
	}
	s_Scene.Get<WaterComponent>(s_WaterEntity).Settings = s_WaterSettings;
	if (s_WaterSettings.Center != before.Center || s_WaterSettings.Height != before.Height || s_WaterSettings.Size != before.Size ||
		s_WaterSettings.Rotation != before.Rotation)
	{
		s_Scene.SetWorldTransform(s_WaterEntity, s_WaterSettings.GetTransform());
	}
}

// Adds the water (one per scene) with these settings and selects it
void CreateWaterEntity(const EnvMapVulkanWaterSettings& settings)
{
	if (s_Scene.FindFirst<WaterComponent>() != NoEntity)
	{
		return;
	}
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Water");
	s_Scene.Add<WaterComponent>(entity).Settings = settings;
	s_Scene.SetWorldTransform(entity, settings.GetTransform());
	s_EditorSelection.SetPrimary(entity);
	ExtractWater();
}

void RemoveWaterEntity()
{
	if (s_WaterEntity != NoEntity)
	{
		s_Scene.DestroyEntity(s_WaterEntity);
		if (s_EditorSelection.GetPrimary() == s_WaterEntity)
		{
			s_EditorSelection.SetPrimary(NoEntity);
		}
	}
	ExtractWater();
}

// Writes an edited s_ProbeVolumeSettings back into the entity (before: as it was read); the placement only when it changed
void CommitProbeVolume(const EnvMapVulkanProbeVolumeSettings& before)
{
	if (s_ProbeVolumeEntity == NoEntity || !s_Scene.Exists(s_ProbeVolumeEntity))
	{
		return;
	}
	s_ProbeVolumeSettings.Size = glm::max(s_ProbeVolumeSettings.Size, glm::vec3(0.1f));
	s_ProbeVolumeSettings.Counts = s_ProbeVolumeSettings.GetCounts();
	s_Scene.Get<ProbeVolumeComponent>(s_ProbeVolumeEntity).Settings = s_ProbeVolumeSettings;
	if (s_ProbeVolumeSettings.Center != before.Center || s_ProbeVolumeSettings.Size != before.Size)
	{
		s_Scene.SetWorldTransform(s_ProbeVolumeEntity, GetProbeVolumeTransform(s_ProbeVolumeSettings));
	}
}

// Adds the probe volume (one per scene) with these settings and selects it
static void CreateProbeVolumeEntity(const EnvMapVulkanProbeVolumeSettings& settings)
{
	if (s_Scene.FindFirst<ProbeVolumeComponent>() != NoEntity)
	{
		return;
	}
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Probe Volume");
	s_Scene.Add<ProbeVolumeComponent>(entity).Settings = settings;
	s_Scene.SetWorldTransform(entity, GetProbeVolumeTransform(settings));
	s_EditorSelection.SetPrimary(entity);
	ExtractProbeVolume();
}

void RemoveProbeVolumeEntity()
{
	if (s_ProbeVolumeEntity != NoEntity)
	{
		s_Scene.DestroyEntity(s_ProbeVolumeEntity);
		if (s_EditorSelection.GetPrimary() == s_ProbeVolumeEntity)
		{
			s_EditorSelection.SetPrimary(NoEntity);
		}
	}
	ExtractProbeVolume();
}

// The box around every mesh of every model of the scene, in the world (false: the scene has no models)
bool GetSceneModelBounds(glm::vec3& boundsMin, glm::vec3& boundsMax)
{
	boundsMin = glm::vec3(std::numeric_limits<float>::max());
	boundsMax = glm::vec3(-std::numeric_limits<float>::max());
	bool any = false;
	for (EnvMapVulkanEntityID entity : GetModels(s_Scene))
	{
		H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, entity);
		const glm::mat4 world = s_Scene.GetWorldTransform(entity);
		for (const H2M::RefH2M<H2M::MeshH2M>& mesh : model->GetMeshes())
		{
			const glm::mat4 transform = GetMeshTransform(model, mesh, world);
			for (int c = 0; c < 8; c++)
			{
				const glm::vec3 corner((c & 1) ? mesh->BoundingBox.Max.x : mesh->BoundingBox.Min.x, (c & 2) ? mesh->BoundingBox.Max.y : mesh->BoundingBox.Min.y,
					(c & 4) ? mesh->BoundingBox.Max.z : mesh->BoundingBox.Min.z);
				const glm::vec3 position = glm::vec3(transform * glm::vec4(corner, 1.0f));
				boundsMin = glm::min(boundsMin, position);
				boundsMax = glm::max(boundsMax, position);
				any = true;
			}
		}
	}
	return any;
}



// Selects a light of s_Lights; None deselects the selected light (a selected model stays selected)
void SelectLight(LightKind kind, int index)
{
	if (kind != LightKind::None)
	{
		s_EditorSelection.SetPrimary(s_LightEntities.Get(kind, index));
	}
	else if (s_SelectedLightKind != LightKind::None)
	{
		s_EditorSelection.SetPrimary(NoEntity);
	}
	s_SelectedLightKind = s_EditorSelection.GetPrimary() != NoEntity ? kind : LightKind::None;
	s_SelectedLightIndex = index;
}

void SelectWater()
{
	if (s_WaterEntity != NoEntity)
	{
		s_EditorSelection.SetPrimary(s_WaterEntity);
	}
	ExtractLights();
	ExtractWater();
}

// Called once per frame, before the viewport's gizmos and picking: a selection that no longer exists (a removed entity)
// is cleared, and the lights and the water are read again, so the selected light's kind and index and the water's
// selection follow the selected entity
void SyncLightSelection()
{
	s_EditorSelection.Sync(); // removed entities leave the selection
	ExtractLights();
	ExtractWater();
}

// Points the sun at the brightest light source of the environment map (the sun of an outdoor HDR, a bright window
// indoors) and gives it that light's color. The intensity stays as it is.
void AlignSunToEnvironment()
{
	if (!EnvMapVulkanRenderer::HasEnvironmentMap())
	{
		return;
	}
	const EnvMapVulkanDirectionalLight before = s_Lights.Sun;
	// The sun taken out of the map: its direction, color and intensity (the directional sun now carries its light); without
	// one, the map's brightest region (a window, bright sky): direction and color only
	EnvMapVulkanExtractedSun mapSun;
	if (!EnvMapVulkanRenderer::FindEnvironmentSun(mapSun))
	{
		Log::GetLogger()->warn("Align to Environment: the environment map's pixels are not available");
		return;
	}
	const glm::vec3 mapDirection = mapSun.Direction;
	const glm::vec3 color = mapSun.Color;
	if (mapSun.Found)
	{
		s_Lights.Sun.Intensity = mapSun.Intensity;
	}

	// The shaders look the environment up at RotateVectorAboutY(rotation, worldDirection): undo that rotation
	float angle = glm::radians(GetEnvironmentRotation());
	float c = std::cos(angle), s = std::sin(angle);
	glm::vec3 worldDirection(c * mapDirection.x + s * mapDirection.z, mapDirection.y, -s * mapDirection.x + c * mapDirection.z);

	s_Lights.Sun.SetDirection(worldDirection);
	s_Lights.Sun.Color = color;
	CommitSun(before);
	Log::GetLogger()->info("Sun aligned to the environment: azimuth {0}, elevation {1}, color ({2}, {3}, {4}), intensity {5}",
		s_Lights.Sun.Azimuth, s_Lights.Sun.Elevation, color.r, color.g, color.b, s_Lights.Sun.Intensity);
}

// Where a new light goes: where the camera looks, so the light is within reach of its default range:
// - the model surface at the center of the viewport
// - else the ground (y = 0) there, when it's not too far
// - else (the camera looks over the scene, e.g. horizontally) the point of the view ray nearest the models' center,
//   at the models' height
// The range of a new point or spot light at this position: far enough to reach every loaded model (the distance to the
// farthest corner of their bounds), at least 10, at most 1000 (the Range drag's limit). Without models: 10.
float GetDefaultLightRange(const glm::vec3& position)
{
	float range = 10.0f;
	glm::vec3 boundsMin, boundsMax;
	EnvMapVulkanRenderer::GetShadowCasterBounds(boundsMin, boundsMax);
	if (glm::all(glm::lessThanEqual(boundsMin, boundsMax)))
	{
		for (int c = 0; c < 8; c++)
		{
			glm::vec3 corner((c & 1) ? boundsMax.x : boundsMin.x, (c & 2) ? boundsMax.y : boundsMin.y, (c & 4) ? boundsMax.z : boundsMin.z);
			range = std::max(range, glm::length(corner - position));
		}
	}
	return std::min(range, 1000.0f);
}

glm::vec3 GetLightSpawnTarget()
{
	EnvMapVulkanEntityID hitModel;
	int hitMesh;
	glm::vec3 target;
	RaycastMesh(0.0f, 0.0f, hitModel, hitMesh, &target);
	if (hitModel == NoEntity)
	{
		glm::vec3 origin, direction;
		GetCameraRay(0.0f, 0.0f, origin, direction);
		direction = glm::normalize(direction);
		float groundT = direction.y < -1e-4f ? -origin.y / direction.y : -1.0f;
		if (groundT > 0.0f && groundT < 50.0f)
		{
			target = origin + direction * groundT;
		}
		else
		{
			glm::vec3 sceneCenter(0.0f);
			const std::vector<EnvMapVulkanEntityID> models = GetModels(s_Scene);
			for (EnvMapVulkanEntityID model : models)
			{
				sceneCenter += glm::vec3(s_Scene.GetWorldTransform(model)[3]) / (float)models.size();
			}
			float t = glm::max(glm::dot(sceneCenter - origin, direction), 1.0f);
			target = origin + direction * t;
			target.y = sceneCenter.y;
		}
	}
	return glm::any(glm::isnan(target)) ? glm::vec3(0.0f) : target; // NaN: the camera has no valid view matrix yet
}

EntityKind GetEntityKind(EnvMapVulkanEntityID entity)
{
	if (s_Scene.Has<ModelComponent>(entity))       return EntityKind::Model;
	if (s_Scene.Has<MeshPartComponent>(entity))    return EntityKind::Part;
	if (s_Scene.Has<PointLightComponent>(entity))  return EntityKind::PointLight;
	if (s_Scene.Has<SpotLightComponent>(entity))   return EntityKind::SpotLight;
	if (s_Scene.Has<SunComponent>(entity))         return EntityKind::Sun;
	if (s_Scene.Has<WaterComponent>(entity))       return EntityKind::Water;
	if (s_Scene.Has<ProbeVolumeComponent>(entity)) return EntityKind::ProbeVolume;
	if (s_Scene.Has<EnvironmentComponent>(entity)) return EntityKind::Environment;
	if (s_Scene.Has<CameraComponent>(entity))      return EntityKind::Camera;
	if (s_Scene.Has<AudioSourceComponent>(entity)) return EntityKind::Sound; // a sound alone (a model's is the model's)
	return EntityKind::Other;
}

const char* GetEntityKindName(EntityKind kind)
{
	switch (kind)
	{
		case EntityKind::Environment: return "Environment";
		case EntityKind::Sun:         return "Sun";
		case EntityKind::Camera:      return "Camera";
		case EntityKind::PointLight:  return "Point Light";
		case EntityKind::SpotLight:   return "Spot Light";
		case EntityKind::Water:       return "Water";
		case EntityKind::ProbeVolume: return "Probe Volume";
		case EntityKind::Model:       return "Model";
		case EntityKind::Part:        return "Mesh (part of a model)";
		case EntityKind::Sound:       return "Sound";
		default:                      return "Entity";
	}
}

// The marker drawn in front of an entity's name in the hierarchy
ImU32 GetEntityKindColor(EntityKind kind)
{
	switch (kind)
	{
		case EntityKind::Environment: return IM_COL32(110, 200, 120, 255);
		case EntityKind::Sun:         return IM_COL32(255, 210, 60, 255);
		case EntityKind::Camera:      return IM_COL32(240, 120, 170, 255);
		case EntityKind::PointLight:  return IM_COL32(255, 150, 60, 255);
		case EntityKind::SpotLight:   return IM_COL32(90, 200, 255, 255);
		case EntityKind::Water:       return IM_COL32(60, 120, 230, 255);
		case EntityKind::ProbeVolume: return IM_COL32(190, 130, 255, 255);
		case EntityKind::Model:       return IM_COL32(200, 200, 210, 255);
		case EntityKind::Part:        return IM_COL32(140, 140, 150, 255);
		case EntityKind::Sound:       return IM_COL32(80, 220, 200, 255);
		default:                      return IM_COL32(128, 128, 128, 255);
	}
}

// Entities that can be dragged under another entity, and the ones that can take children. A part stays in its model, the
// environment, the sun and the water stay at the top (the water is always level: a parent could tilt it).
bool CanBeReparented(EntityKind kind)
{
	return kind == EntityKind::Model || kind == EntityKind::PointLight || kind == EntityKind::SpotLight || kind == EntityKind::Sound;
}

bool CanHaveChildren(EntityKind kind)
{
	return kind == EntityKind::Model || kind == EntityKind::Part || kind == EntityKind::PointLight || kind == EntityKind::SpotLight;
}

bool CanBeDeleted(EntityKind kind)
{
	return kind == EntityKind::Model || kind == EntityKind::Part || kind == EntityKind::PointLight || kind == EntityKind::SpotLight ||
		kind == EntityKind::Water || kind == EntityKind::ProbeVolume || kind == EntityKind::Sound;
}

// Removes an entity (and its children): a model or a part at the start of the next frame (its GPU resources may be in use),
// a light or the water right away. A model's last mesh takes the model with it (see EditorPreFrame).
void DeleteEntity(EnvMapVulkanEntityID entity)
{
	switch (GetEntityKind(entity))
	{
		case EntityKind::Model:
		case EntityKind::Part:
			s_PendingRemoveEntities.push_back(entity);
			break;
		case EntityKind::PointLight:
		case EntityKind::SpotLight:
			s_Scene.DestroyEntity(entity);
			s_EditorSelection.Sync();
			ExtractLights();
			break;
		case EntityKind::Water:
			RemoveWaterEntity();
			break;
		case EntityKind::ProbeVolume:
			RemoveProbeVolumeEntity();
			break;
		case EntityKind::Sound:
			s_Scene.DestroyEntity(entity);
			s_EditorSelection.Sync();
			break;
		default:
			break;
	}
}

// A sound alone (5.3): an entity with an Audio Source, where the camera looks, selected. Its clip is chosen in the Inspector
void AddSoundAtView()
{
	const EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Sound");
	s_Scene.Get<TransformComponent>(entity).Translation = GetLightSpawnTarget() + glm::vec3(0.0f, 1.0f, 0.0f);
	AudioSourceComponent& source = s_Scene.Add<AudioSourceComponent>(entity);
	source.Loop = true;
	source.PlayOnStart = true;
	s_EditorSelection.SetPrimary(entity);
}

// New lights and the water go where the camera looks (see GetLightSpawnTarget), selected
void AddPointLightAtView()
{
	if (EnvMapVulkanPointLight* light = s_Lights.AddPointLight(GetLightSpawnTarget() + glm::vec3(0.0f, 1.5f, 0.0f), MaxShadowedPointLights))
	{
		light->Range = GetDefaultLightRange(light->Position);
		s_EditorSelection.SetPrimary(CreatePointLightEntity(*light));
	}
	ExtractLights();
}

void AddSpotLightAtView()
{
	if (EnvMapVulkanSpotLight* light = s_Lights.AddSpotLight(GetLightSpawnTarget() + glm::vec3(0.0f, 3.0f, 0.0f), MaxShadowedSpotLights))
	{
		light->Range = GetDefaultLightRange(light->Position);
		s_EditorSelection.SetPrimary(CreateSpotLightEntity(*light));
	}
	ExtractLights();
}

void AddWaterAtView()
{
	glm::vec3 target = GetLightSpawnTarget();
	EnvMapVulkanWaterSettings settings;
	settings.Center = glm::vec2(target.x, target.z);
	CreateWaterEntity(settings);
}

// The probe volume goes around where the camera looks, standing on that point's height
void AddProbeVolumeAtView()
{
	EnvMapVulkanProbeVolumeSettings settings;
	const glm::vec3 target = GetLightSpawnTarget();
	settings.Center = target + glm::vec3(0.0f, settings.Size.y * 0.5f, 0.0f);
	CreateProbeVolumeEntity(settings);
}
