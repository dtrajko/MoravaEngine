#pragma once

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <cstdint>
#include <vector>


class EnvMapVulkanScene;
class InputSystem;
class ScenePhysics;

/**
 * What the scripts' C# API works on while a game runs (docs/engine/Game_Engine_Runtime_Guide.html, 4.4): the running
 * game's scene, its physics and its input, and where the game loop is. GameSystems owns it and keeps it current.
 */
struct ScriptContext
{
	EnvMapVulkanScene* Scene = nullptr;
	ScenePhysics* Physics = nullptr;
	InputSystem* Input = nullptr;   // nullptr: no input (Simulate, where scripts don't run)

	bool InFixedStep = false;       // in OnFixedUpdate: Input's edges and Time.DeltaTime are the fixed step's
	float DeltaTime = 0.0f;         // the current update's
	float FixedDeltaTime = 0.0f;
	double Time = 0.0;              // the fixed steps' game time
	uint64_t FrameCount = 0, FixedStepCount = 0;

	std::vector<EnvMapVulkanEntityID> DestroyQueue; // Entity.Destroy: at the end of the update it was called in
};

/**
 * The C++ side of the scripts' API: the functions of ScriptNativeApi that work on a running game (ScriptApi.h). Bind puts
 * them into ScriptHost's table; SetContext tells them which game (nullptr: none, and the calls do nothing or return "no
 * entity").
 */
namespace ScriptGlue
{
	void Bind();
	void SetContext(ScriptContext* context);

	// Destroys the entities Entity.Destroy asked for (with their children); their physics bodies go at the next step
	void DestroyQueued(ScriptContext& context);
}
