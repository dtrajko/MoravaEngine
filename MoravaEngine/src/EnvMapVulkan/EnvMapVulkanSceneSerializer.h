#pragma once

#include "EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkanScene.h"

#include <yaml-cpp/yaml.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * Scene files (.mscene, YAML) of SceneEnvMapVulkan: the scene's name, its entities with their components, and how the
 * renderer draws it (EnvMapVulkanRenderSettings; exposure is the environment's).
 *
 *   Scene: { Version, Name }
 *   EditorCamera: { Translation, Rotation, FieldOfView }   (the editor's view, 6.3; the player doesn't read it)
 *   RenderSettings: { Shadows, LocalShadows, RayTracing, Reflections, AmbientOcclusion, Bloom }
 *   Entities:
 *     - ID, Name, Parent (0: top level), Transform { Translation, Rotation, Scale }
 *       and the components it has: Model { File }, MeshPart { SourceMeshIndex, Material { ID, File } }, Sun, PointLight,
 *       SpotLight, Water, Environment, Camera { Primary, FieldOfView, NearClip, FarClip }, physics, Script, AudioSource...
 *
 * Entities are written parents first (the order of the hierarchy), so a parent exists when its children are read.
 * Paths are written and read through ProjectPaths: relative to the open project's folder or to the engine's.
 *
 * Reading creates the entities and their components; what needs the renderer is left to the caller: the models are not
 * loaded (ModelComponent::Model is null; the parts say which meshes and materials they use), and the materials are only
 * named (MeshPartRefs: the ID and the file of each part's material). EnvMapVulkanSceneLoader does both.
 */
namespace EnvMapVulkanSceneSerializer
{
	constexpr const char* ScenesFolder = "Scenes";
	constexpr const char* FileExtension = ".mscene";

	// A part's material as the file names it, until the caller finds or loads it
	struct MaterialRef
	{
		uint64_t ID = 0;
		std::string File;
	};

	// Where the editor looks at the scene from (its own camera, not an entity): saved with the scene, so an opened scene is
	// seen as it was. Rotation as a camera entity's: x down (positive) and up, y around the vertical axis
	struct EditorView
	{
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::vec3 Rotation = glm::vec3(0.0f);
		float FieldOfView = 45.0f;
	};

	// The scene as text (the file's content), with the editor's view when given
	std::string Serialize(EnvMapVulkanScene& scene, const std::string& sceneName, const EnvMapVulkanRenderSettings& renderSettings,
		const EditorView* editorView = nullptr);
	// The editor's view a scene file was saved with: false when it has none (saved before 6.3, or by the player)
	bool ReadEditorView(const std::string& filepath, EditorView& view);

	// Prefab files (.mprefab, 6.1): "Prefab: { Version, Name }" and the prefab's entities as a scene file writes them, the
	// root first (Parent 0), with IDs that only number them within the file (EnvMapVulkanPrefab makes them)
	constexpr const char* PrefabExtension = ".mprefab";
	constexpr const char* PrefabsFolder = "Prefabs"; // in the project's asset folder
	// The text of a prefab file: the scene holds the prefab's entities only, under one root
	std::string SerializePrefab(EnvMapVulkanScene& prefab, const std::string& name);
	// Reads a prefab file's entities into the scene under parent. newId gives each its ID by its place in the file (0: the
	// root; NoEntity or nullptr: a new random one); script fields that point at another of them point at its new ID.
	// created gets them in the file's order (created[0]: the root), materialRefs each part's material. False (error says
	// why) when the file can't be read; then nothing was created
	bool DeserializePrefab(const std::string& filepath, EnvMapVulkanScene& scene, EnvMapVulkanEntityID parent,
		const std::function<EnvMapVulkanEntityID(size_t)>& newId, std::unordered_map<EnvMapVulkanEntityID, MaterialRef>& materialRefs,
		std::vector<EnvMapVulkanEntityID>& created, std::string& error);

	// Reads a scene file into the (empty) scene. sceneName, renderSettings and each part's material reference (by entity)
	// come back to the caller. False (error explains) when the file can't be read.
	bool Deserialize(const std::string& filepath, EnvMapVulkanScene& scene, std::string& sceneName, EnvMapVulkanRenderSettings& renderSettings,
		std::unordered_map<EnvMapVulkanEntityID, MaterialRef>& materialRefs, std::string& error);
}
