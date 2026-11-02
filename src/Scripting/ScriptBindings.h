#pragma once

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>


class EnvMapVulkanScene;
class GameSettings;
class InputSystem;
class SceneAnimation;
class SceneAudio;
class ScenePhysics;
class ScriptRuntime;
class SceneUI;

/**
 * What the scripts' C# API works on while a game runs (docs/engine/Game_Engine_Runtime_Guide.html, 4.4): the running
 * game's scene, its physics, its sounds, its animators, its scripts, its input and its UI, and where the game loop is. GameSystems
 * owns it and keeps it current.
 */
struct ScriptContext
{
	EnvMapVulkanScene* Scene = nullptr;
	ScenePhysics* Physics = nullptr;
	SceneAudio* Audio = nullptr;
	SceneAnimation* Animation = nullptr;
	ScriptRuntime* Scripts = nullptr;
	InputSystem* Input = nullptr;   // nullptr: no input (Simulate, where scripts don't run)
	SceneUI* UI = nullptr;          // the game UI's buttons and selection (7.3)
	GameSettings* Settings = nullptr; // what the game remembers between runs (Milestone 2: the C# Settings class)

	// The scene's name and file (relative to the project; empty: never saved), and the one Scene.Load asked for (6.2): done
	// by the game's owner at the end of the frame (GameSystems::TakeSceneLoad). An empty path: the current scene again
	std::string SceneName, ScenePath;
	bool SceneLoadRequested = false;
	std::string SceneLoadPath;
	bool QuitRequested = false;     // Application.Quit: the owner ends the game after this frame (GameSystems::TakeQuit)
	bool Fullscreen = false;        // the window fills its monitor now (the owner says so each frame)
	int FullscreenRequest = -1;     // Application.Fullscreen set: 0 or 1, taken by the owner after the frame (-1: none)

	bool InFixedStep = false;       // in OnFixedUpdate: Input's edges and Time.DeltaTime are the fixed step's
	float DeltaTime = 0.0f;         // the current update's
	float FixedDeltaTime = 0.0f;
	double Time = 0.0;              // the fixed steps' game time
	uint64_t FrameCount = 0, FixedStepCount = 0;
	float TimeScale = 1.0f;         // Time.TimeScale: game time per real time (kept when a scene is loaded or reloaded)
	float UnscaledDeltaTime = 0.0f; // the frame's real time (a fixed step's: the step)
	double UnscaledTime = 0.0;      // real time since the game started (the frames')
	bool CursorLocked = false;      // Input.CursorLocked: the game wants the mouse (kept when a scene is loaded or reloaded)

	std::vector<EnvMapVulkanEntityID> DestroyQueue; // Entity.Destroy: at the end of the update it was called in
	std::set<std::string> InstantiatedPrefabs;     // Scene.Instantiate: the files placed (the first of each is logged)
};

/**
 * The C++ side of the scripts' API: the functions of ScriptNativeApi that work on a running game (ScriptApi.h). Bind puts
 * them into ScriptHost's table; SetContext tells them which game (nullptr: none, and the calls do nothing or return "no
 * entity").
 */
namespace ScriptBindings
{
	void Bind();
	void SetContext(ScriptContext* context);

	// Destroys the entities Entity.Destroy asked for (with their children); their physics bodies go at the next step
	void DestroyQueued(ScriptContext& context);
}
