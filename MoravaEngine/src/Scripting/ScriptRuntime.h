#pragma once

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include "Animation/SceneAnimation.h"
#include "Physics/ScenePhysics.h"

#include "UI/SceneUI.h"

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


class EnvMapVulkanScene;

/**
 * The scripts of a running game (docs/engine/Game_Engine_Runtime_Guide.html, 4.5): an instance of its class for each
 * entity with a ScriptComponent, called by GameSystems at its place in the game loop. Start makes them all, gives each the
 * fields the scene sets, then calls each one's OnCreate, so a script's OnCreate can use any other script's fields. The
 * collisions and triggers of a physics step go to the scripts of both entities. An instance whose entity is gone (a
 * script destroyed it, or the editor removed it during Play) is dropped. Entities placed while the game runs (a prefab,
 * Scene.Instantiate: 6.2) get theirs at once, the same way: AddEntities.
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
	// Entities new in the running game (in the Scene Hierarchy's order): the scripts of those that have one, with their
	// fields, then each one's OnCreate, before it returns
	void AddEntities(const std::vector<EnvMapVulkanEntityID>& entities);
	bool IsRunning() const { return m_Scene != nullptr; }

	void FixedUpdate(float fixedStep);
	void Update(float frameTime);
	// The step's collision and trigger events (ScenePhysics::GetEvents), to both entities' scripts
	void DispatchEvents(const std::vector<ScenePhysics::Event>& events);
	// The frame's animation events (SceneAnimation::GetEvents), to the animated entities' scripts
	void DispatchAnimationEvents(const std::vector<SceneAnimation::Event>& events);
	// The frame's button clicks (SceneUI::GetClicks): each button's On Click method (UIButtonComponent), then its C#
	// listeners (button.Clicked)
	void DispatchButtonClicks(const std::vector<SceneUI::Click>& clicks);
	// The frame's slider changes (SceneUI::GetValueChanges): each slider's On Value Changed method (UISliderComponent),
	// then its C# listeners (slider.ValueChanged)
	void DispatchSliderChanges(const std::vector<SceneUI::ValueChange>& changes);
	// Drops the instances of entities that are gone
	void RemoveDestroyed();

	// The instance of an entity's script (nullptr: none): the Inspector shows its fields as they are now
	void* GetInstance(EnvMapVulkanEntityID entity) const;
	size_t GetInstanceCount() const { return m_Instances.size(); }

	// How many calls threw since Start, and the last one: "Sample.PlayerController.OnFixedUpdate"
	uint64_t GetExceptionCount() const { return m_ExceptionCount; }
	const std::string& GetLastException() const { return m_LastException; }

	struct Instance
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		void* Handle = nullptr;
		std::string ClassName;
		// Its calls that threw, and the last one's method ("OnFixedUpdate"): the Scripts panel shows them
		mutable uint64_t ExceptionCount = 0;
		mutable std::string LastExceptionMethod;
	};
	// In the order they were made: the order they are called in
	const std::vector<Instance>& GetInstances() const { return m_Instances; }

private:
	// Instances of the entities' scripts, their fields, then their OnCreate (atStart: logged as the game's scripts)
	void CreateInstances(const std::vector<EnvMapVulkanEntityID>& scripted, bool atStart);
	void ApplyFields(const Instance& instance, const ScriptComponent& script);
	void Threw(const Instance& instance, const char* method);
	// Counted as an exception without an instance (what: "UIButton.Clicked")
	void Threw(const std::string& what, EnvMapVulkanEntityID entity);
	const Instance* Find(EnvMapVulkanEntityID entity) const;
	// A UI element's method (a button's On Click, a slider's On Value Changed): of target's script, or (no target) of the
	// first script from the element up to its canvas that has it; value: the slider's (nullptr: a method without
	// parameters). Logged once per element and method when no script has it
	void CallUIMethod(EnvMapVulkanEntityID element, EnvMapVulkanEntityID target, const std::string& method, const float* value);

	EnvMapVulkanScene* m_Scene = nullptr;
	std::vector<Instance> m_Instances; // in the order they were made: the order they are called in
	std::unordered_map<EnvMapVulkanEntityID, size_t> m_ByEntity;
	uint64_t m_ExceptionCount = 0;
	std::string m_LastException;
	std::set<std::pair<EnvMapVulkanEntityID, std::string>> m_MissingMethods; // a button's method no script has: logged once
};
