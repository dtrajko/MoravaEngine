#pragma once

#include "GameLoop.h"

#include "Input/InputMap.h"


class EnvMapVulkanScene;
class InputSystem;

/**
 * The game's systems, run by the game loop on the running game's scene: the same in the player and in the editor's Play.
 * Now the input test (InputTestComponent); physics, scripts, audio and animation come with their phases.
 */
class GameSystems : public GameLoopSystems
{
public:
	// The scene and the input the systems work with (the game starts: InputTestComponent's runtime state is fresh)
	void Begin(EnvMapVulkanScene* scene, InputSystem* input);

	virtual void FixedUpdate(float fixedStep) override;
	virtual void Update(float frameTime) override;

	// The game's actions: the open project's input file, else (no project, a project without one, or a file that can't be
	// read: logged) InputMap::Default
	static InputMap LoadInputMap();

private:
	EnvMapVulkanScene* m_Scene = nullptr;
	InputSystem* m_Input = nullptr;
};
