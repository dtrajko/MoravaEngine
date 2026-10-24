#pragma once

#include "GameLoop.h"

#include "Audio/SceneAudio.h"

#include "Input/InputMap.h"

#include "Physics/ScenePhysics.h"

#include "Scripting/ScriptBindings.h"
#include "Scripting/ScriptRuntime.h"


class EnvMapVulkanScene;
class InputSystem;

/**
 * The game's systems, run by the game loop on the running game's scene: the same in the player and in the editor's Play.
 * Now the input test (InputTestComponent), physics (ScenePhysics), the C# scripts (ScriptRuntime) and what their API works
 * on (ScriptContext: time, and the entities scripts destroy, at the end of each update), and the sounds (SceneAudio, 5.1:
 * at the end of each frame, where the frame left the entities); animation comes with its phase.
 *
 * A fixed step: the scripts' OnFixedUpdate, what they destroyed, the physics step, its collisions and triggers to the
 * scripts, what those destroyed. A frame: the bodies' interpolated transforms, the scripts' OnUpdate, what it destroyed.
 */
class GameSystems : public GameLoopSystems
{
public:
	// The scene and the input the systems work with (the game starts: InputTestComponent's runtime state is fresh, the
	// physics bodies are made, then the scripts' instances). Begin(nullptr, nullptr): the game is over (the scripts and
	// the bodies are gone). Begin(scene, nullptr): Simulate (3.7): physics only, without the game's logic (the input test
	// and the scripts); a character stands where it is, since its game moves it.
	void Begin(EnvMapVulkanScene* scene, InputSystem* input);
	~GameSystems() override { ScriptBindings::SetContext(nullptr); }

	virtual void FixedUpdate(float fixedStep) override;
	virtual void Update(float frameTime, float alpha) override;

	ScenePhysics& GetPhysics() { return m_Physics; }
	ScriptRuntime& GetScripts() { return m_Scripts; }
	SceneAudio& GetAudio() { return m_Audio; }
	const ScriptContext& GetScriptContext() const { return m_ScriptContext; }

	// The game's actions: the open project's input file, else (no project, a project without one, or a file that can't be
	// read: logged) InputMap::Default
	static InputMap LoadInputMap();

	// The game's scripts: the open project's script assembly (Project Settings: Script Assembly), after the one loaded
	// before is unloaded. No project, or a project without one: no scripts. Not while a game runs (its instances hold the
	// assembly)
	static void LoadScripts();

private:
	EnvMapVulkanScene* m_Scene = nullptr;
	InputSystem* m_Input = nullptr;
	ScenePhysics m_Physics;
	ScriptContext m_ScriptContext; // what the scripts' C# API works on (ScriptBindings)
	ScriptRuntime m_Scripts;
	SceneAudio m_Audio;
};
