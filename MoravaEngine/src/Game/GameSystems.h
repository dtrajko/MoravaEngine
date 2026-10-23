#pragma once

#include "GameLoop.h"

#include "Input/InputMap.h"

#include "Physics/ScenePhysics.h"

#include "Scripting/ScriptGlue.h"


class EnvMapVulkanScene;
class InputSystem;

/**
 * The game's systems, run by the game loop on the running game's scene: the same in the player and in the editor's Play.
 * Now the input test (InputTestComponent), physics (ScenePhysics) and what the scripts' C# API works on (ScriptContext:
 * time, and the entities scripts destroy, at the end of each update); the scripts themselves, audio and animation come
 * with their phases.
 */
class GameSystems : public GameLoopSystems
{
public:
	// The scene and the input the systems work with (the game starts: InputTestComponent's runtime state is fresh, the
	// physics bodies are made). Begin(nullptr, nullptr): the game is over (the bodies are gone). Begin(scene, nullptr):
	// Simulate (3.7): physics only, without the game's logic (the input test now, scripts from Phase 4); a character
	// stands where it is, since its game moves it.
	void Begin(EnvMapVulkanScene* scene, InputSystem* input);
	~GameSystems() override { ScriptGlue::SetContext(nullptr); }

	virtual void FixedUpdate(float fixedStep) override;
	virtual void Update(float frameTime, float alpha) override;

	ScenePhysics& GetPhysics() { return m_Physics; }
	const ScriptContext& GetScriptContext() const { return m_ScriptContext; }

	// The game's actions: the open project's input file, else (no project, a project without one, or a file that can't be
	// read: logged) InputMap::Default
	static InputMap LoadInputMap();

private:
	EnvMapVulkanScene* m_Scene = nullptr;
	InputSystem* m_Input = nullptr;
	ScenePhysics m_Physics;
	ScriptContext m_ScriptContext; // what the scripts' C# API works on (ScriptGlue)
};
