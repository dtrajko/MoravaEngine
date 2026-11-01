#pragma once

#include "GameLoop.h"
#include "GameSettings.h"

#include "Animation/SceneAnimation.h"

#include "Audio/SceneAudio.h"

#include "Input/InputMap.h"

#include "Physics/ScenePhysics.h"

#include "Scripting/ScriptBindings.h"
#include "Scripting/ScriptRuntime.h"

#include "UI/SceneUI.h"


class EnvMapVulkanScene;
class InputSystem;

/**
 * The game's systems, run by the game loop on the running game's scene: the same in the player and in the editor's Play.
 * Now the input test (InputTestComponent), physics (ScenePhysics), the C# scripts (ScriptRuntime) and what their API works
 * on (ScriptContext: time, and the entities scripts destroy, at the end of each update), the game UI's buttons (SceneUI,
 * 7.3: each frame, before the scripts' OnUpdate, in real time), the animators (SceneAnimation, 6.4: each frame, after the
 * scripts set their parameters; also in Simulate) and the sounds (SceneAudio, 5.1: at the end of each frame, where the
 * frame left the entities).
 *
 * A fixed step: the scripts' OnFixedUpdate, what they destroyed, the physics step, its collisions and triggers to the
 * scripts, what those destroyed. A frame: the bodies' interpolated transforms, the UI's clicks to the scripts, the
 * scripts' OnUpdate, what it destroyed, the animators' poses, the sounds.
 *
 * Scene.Load (6.2) is the owner's to do at the end of the frame (the player, the editor's Play: they know where the scene
 * goes): TakeSceneLoad says what was asked for, the owner opens it into the same scene and calls Restart. Application.Quit
 * (Milestone 2) is the owner's too: TakeQuit says it was asked for (the player closes, the editor stops Play).
 *
 * The game's settings (GameSettings, the C# Settings class): the owner opens their file before Begin; they stay while
 * scenes load, and are saved when the game is over (Begin(nullptr, nullptr)).
 */
class GameSystems : public GameLoopSystems
{
public:
	// The scene and the input the systems work with (the game starts: InputTestComponent's runtime state is fresh, the
	// physics bodies are made, then the scripts' instances). Begin(nullptr, nullptr): the game is over (the scripts and
	// the bodies are gone). Begin(scene, nullptr): Simulate (3.7): physics only, without the game's logic (the input test
	// and the scripts); a character stands where it is, since its game moves it.
	void Begin(EnvMapVulkanScene* scene, InputSystem* input);
	~GameSystems() override;
	// The scene's name and file (absolute; empty: never saved), for the scripts' Scene.Name and Scene.Path: before Begin
	// and Restart (OnCreate may ask for them)
	void SetSceneInfo(const std::string& name, const std::string& path);

	// Scene.Load asked for a scene in this frame: true once, with its file (absolute), or an empty path for the current
	// scene again (Scene.Reload)
	bool TakeSceneLoad(std::string& path);
	// The game again on its scene, whose content the owner replaced (Scene.Load): bodies, sounds and scripts made anew,
	// the clock going on
	void Restart();
	// Application.Quit asked for the game to end in this frame: true once
	bool TakeQuit();

	// The file of the game's settings (GameSettings::GetUserFile, GetEditorFile; empty: for this run only), before Begin
	void OpenSettings(const std::string& file) { m_Settings.Open(file); }
	const GameSettings& GetSettings() const { return m_Settings; }

	// The mouse over the game's view (view: its size in pixels), each frame before the input is sampled: the mouse buttons
	// the game's UI holds, for InputSystem::SetBlockedMouseButtons (0 without a game, or in Simulate)
	uint32_t UpdateUIPointer(const UIPointer& pointer, const glm::vec2& viewSize);

	virtual void FixedUpdate(float fixedStep) override;
	virtual void Update(float frameTime, float unscaledFrameTime, float alpha) override;
	virtual float GetTimeScale() const override { return m_ScriptContext.TimeScale; }

	ScenePhysics& GetPhysics() { return m_Physics; }
	ScriptRuntime& GetScripts() { return m_Scripts; }
	SceneAudio& GetAudio() { return m_Audio; }
	SceneAnimation& GetAnimation() { return m_Animation; }
	SceneUI& GetUI() { return m_UI; }
	const ScriptContext& GetScriptContext() const { return m_ScriptContext; }

	// The game's actions: the open project's input file, else (no project, a project without one, or a file that can't be
	// read: logged) InputMap::Default
	static InputMap LoadInputMap();

	// The game's scripts: the open project's script assembly (Project Settings: Script Assembly), after the one loaded
	// before is unloaded. No project, or a project without one: no scripts. Not while a game runs (its instances hold the
	// assembly)
	static void LoadScripts();

private:
	void Start(bool keepClock);

	EnvMapVulkanScene* m_Scene = nullptr;
	InputSystem* m_Input = nullptr;
	ScenePhysics m_Physics;
	ScriptContext m_ScriptContext; // what the scripts' C# API works on (ScriptBindings)
	ScriptRuntime m_Scripts;
	SceneAudio m_Audio;
	SceneAnimation m_Animation;
	SceneUI m_UI;
	GameSettings m_Settings;
};
