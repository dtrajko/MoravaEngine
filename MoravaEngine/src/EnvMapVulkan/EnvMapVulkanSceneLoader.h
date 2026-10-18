#pragma once

#include "EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkanScene.h"
#include "EnvMapVulkanSceneSerializer.h"

#include <string>
#include <unordered_map>


/**
 * Scenes and models from files, ready to be drawn: what EnvMapVulkanSceneSerializer leaves to its caller. The models are
 * loaded, and each mesh part gets its material from the Material Library (found by ID, loaded from its file, or imported
 * from the model). Used by the editor and by anything else that opens scenes.
 *
 * Loading a model creates GPU resources; replacing a scene's content destroys the models it had. The caller makes sure no
 * frame in flight still draws them (vkDeviceWaitIdle) before calling Open on a scene that has content.
 */
namespace EnvMapVulkanSceneLoader
{
	// A file the model importer reads (by its extension)
	bool IsModelFile(const std::string& filepath);

	// Loads a model file and adds it to the scene: a model entity named after the file (at the origin, at the top level)
	// with a part entity per mesh, each with the material the model gives it (imported into the Material Library).
	// NoEntity when the file can't be loaded (logged).
	EnvMapVulkanEntityID AddModel(EnvMapVulkanScene& scene, const std::string& filepath);

	// Loads the model of a model entity read from a scene file: the meshes the scene removed are removed again, and each
	// part gets its mesh index, its original transform and its material (by ID; else from its file; else the model's own).
	// False when the model can't be loaded (logged); the entity is left as it is.
	bool AttachModel(EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity,
		const std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanSceneSerializer::MaterialRef>& materialRefs);

	// Opens a scene file: the scene's content is replaced by the file's entities, with their models loaded (an entity
	// whose model can't be loaded is removed, with its children). False (error explains) when the file can't be read;
	// the scene then keeps its content.
	bool Open(const std::string& filepath, EnvMapVulkanScene& scene, std::string& sceneName, EnvMapVulkanRenderSettings& renderSettings,
		std::string& error);
}
