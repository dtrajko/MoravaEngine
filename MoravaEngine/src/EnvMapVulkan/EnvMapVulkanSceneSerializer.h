#pragma once

#include "EnvMapVulkanScene.h"

#include <yaml-cpp/yaml.h>

#include <functional>
#include <string>


/**
 * Scene files (.mscene, YAML) of SceneEnvMapVulkan: the scene's name, its entities with their components, and a section
 * of settings the renderer writes and reads itself (exposure is the environment's; shadows, bloom... are the renderer's).
 *
 *   Scene: { Version, Name }
 *   RenderSettings: { ... }        (written by the caller)
 *   Entities:
 *     - ID, Name, Parent (0: top level), Transform { Translation, Rotation, Scale }
 *       and the components it has: Model { File }, MeshPart { SourceMeshIndex, Material { ID, File } }, Sun, PointLight,
 *       SpotLight, Water, Environment
 *
 * Entities are written parents first (the order of the hierarchy), so a parent exists when its children are read.
 * Paths are relative to the working directory (the project folder) when they are inside it.
 *
 * Reading creates the entities and their components; what needs the renderer is left to the caller: the models are not
 * loaded (ModelComponent::Model is null; the parts say which meshes and materials they use), and the materials are only
 * named (MeshPartRefs: the ID and the file of each part's material).
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

	// The scene as text (the file's content); writeRenderSettings writes the RenderSettings map's content
	std::string Serialize(EnvMapVulkanScene& scene, const std::string& sceneName, const std::function<void(YAML::Emitter&)>& writeRenderSettings);

	// Reads a scene file into the (empty) scene. sceneName, renderSettings (may be null) and each part's material
	// reference (by entity) come back to the caller. False (error explains) when the file can't be read.
	bool Deserialize(const std::string& filepath, EnvMapVulkanScene& scene, std::string& sceneName, YAML::Node& renderSettings,
		std::unordered_map<EnvMapVulkanEntityID, MaterialRef>& materialRefs, std::string& error);

	// A path as stored in scene and material files: relative to the working directory when it is inside it, forward slashes
	std::string ToStoredPath(const std::string& path);
}
