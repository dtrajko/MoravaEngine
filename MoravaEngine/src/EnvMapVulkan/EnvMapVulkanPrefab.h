#pragma once

#include "EnvMapVulkanScene.h"

#include <cstdint>
#include <string>
#include <vector>


/**
 * Prefabs (docs/engine/Game_Engine_Runtime_Guide.html, 6.1): an entity with its descendants saved as a file (.mprefab,
 * EnvMapVulkanSceneSerializer::SerializePrefab), placed in scenes as instances. An instance's root has a PrefabComponent:
 * the file, and two hashes of what the prefab and the instance were when they last matched. The root's name, translation
 * and rotation are the instance's own (a new instance is named after the prefab); everything else is the prefab's.
 *
 * Without per-field overrides (Phase 10) the hashes tell an instance's state: the prefab file changed since (PrefabHash),
 * the instance changed since (InstanceHash), both, or neither. An instance that didn't change takes a changed prefab as it
 * is (Revert); one that did is left alone, so a change made to it is never lost by surprise.
 *
 * Instantiate and Revert load models: the caller makes sure no frame in flight draws the entities they replace.
 */
namespace EnvMapVulkanPrefab
{
	enum class InstanceState
	{
		InSync,          // as the prefab is
		InstanceChanged, // changed since it matched the prefab (Apply writes it to the prefab)
		PrefabChanged,   // the prefab changed since, the instance didn't (Revert takes it, and opening the scene does)
		BothChanged,     // both: Revert loses the instance's changes, Apply the prefab's
		FileMissing,     // the prefab file isn't there (any more)
	};
	const char* GetStateName(InstanceState state);

	// The text a prefab file of the entity has: its subtree, IDs numbered from 1 in the hierarchy's order, the root named
	// name, at the origin and unturned (they are an instance's own), without the root's own PrefabComponent (a nested
	// prefab's stays)
	std::string GetPrefabText(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID root, const std::string& name);
	// 64-bit FNV-1a of a text, its line ends taken as \n (the same on every platform)
	uint64_t HashText(const std::string& text);
	// The hash of the instance's content as a prefab would have it (GetPrefabText, without a name)
	uint64_t GetInstanceHash(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID root);
	// The hash of a prefab file's text; false when it can't be read
	bool GetFileHash(const std::string& file, uint64_t& hash);

	// Writes the entity as a prefab file (its name: the file's), and it becomes an instance of it. False (error says why)
	bool Save(EnvMapVulkanScene& scene, EnvMapVulkanEntityID root, const std::string& file, std::string& error);

	// A new instance of the prefab file under parent, its models loaded (an entity whose model can't be loaded is left
	// out, logged). The root at the origin, as the prefab has it: the caller places it. reuseIds: IDs for the instance's
	// entities in the prefab's order (Revert keeps an instance's IDs, so references to it stay); the rest get new ones.
	// NoEntity (error says why) when the file can't be read
	EnvMapVulkanEntityID Instantiate(EnvMapVulkanScene& scene, const std::string& file, EnvMapVulkanEntityID parent, std::string& error,
		const std::vector<EnvMapVulkanEntityID>& reuseIds = {});

	InstanceState GetState(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID root);

	// The instance again as its prefab is now: its placement, parent, place among its siblings and entity IDs kept.
	// Returns the root (the same ID), or NoEntity (error says why; the instance is left as it was)
	EnvMapVulkanEntityID Revert(EnvMapVulkanScene& scene, EnvMapVulkanEntityID root, std::string& error);

	// The instances' roots of a prefab file in the scene
	std::vector<EnvMapVulkanEntityID> FindInstances(const EnvMapVulkanScene& scene, const std::string& file);

	// Saving, placing, the states, Revert and Apply's reach, on entities without models in a scene of its own, with a
	// prefab file in the system's temporary folder (deleted after). Logs the result (at the renderer's start)
	bool SelfTest();
}
