#pragma once

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include "Physics/ScenePhysics.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>


class EnvMapVulkanScene;

/**
 * The scripts of a running game (docs/engine/Game_Engine_Runtime_Guide.html, 4.5): an instance of its class for each
 * entity with a ScriptComponent, called by GameSystems at its place in the game loop. Start makes them all, gives each the
 * fields the scene sets, then calls each one's OnCreate, so a script's OnCreate can use any other script's fields. The
 * collisions and triggers of a physics step go to the scripts of both entities. An instance whose entity is gone (a
 * script destroyed it, or the editor removed it during Play) is dropped.
 *
 * Every call that throws counts as an exception (C# has logged it with the script's class, method, file and line): the
 * editor pauses Play on it.
 */
class ScriptRuntime
{
public:
	~ScriptRuntime() { Stop(); }

	void Start(EnvMapVulkanScene& scene);
	void Stop();
	bool IsRunning() const { return m_Scene != nullptr; }

	void FixedUpdate(float fixedStep);
	void Update(float frameTime);
	// The step's collision and trigger events (ScenePhysics::GetEvents), to both entities' scripts
	void DispatchEvents(const std::vector<ScenePhysics::Event>& events);
	// Drops the instances of entities that are gone
	void RemoveDestroyed();

	// The instance of an entity's script (nullptr: none): the Properties panel shows its fields as they are now
	void* GetInstance(EnvMapVulkanEntityID entity) const;
	size_t GetInstanceCount() const { return m_Instances.size(); }

	// How many calls threw since Start, and the last one: "Sample.PlayerController.OnFixedUpdate"
	uint64_t GetExceptionCount() const { return m_ExceptionCount; }
	const std::string& GetLastException() const { return m_LastException; }

private:
	struct Instance
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		void* Handle = nullptr;
		std::string ClassName;
	};

	void ApplyFields(const Instance& instance, const ScriptComponent& script);
	void Threw(const Instance& instance, const char* method);
	const Instance* Find(EnvMapVulkanEntityID entity) const;

	EnvMapVulkanScene* m_Scene = nullptr;
	std::vector<Instance> m_Instances; // in the order they were made: the order they are called in
	std::unordered_map<EnvMapVulkanEntityID, size_t> m_ByEntity;
	uint64_t m_ExceptionCount = 0;
	std::string m_LastException;
};
