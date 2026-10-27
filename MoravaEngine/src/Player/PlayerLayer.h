#pragma once

#include "H2M/Core/LayerH2M.h"

#include "Camera/Camera.h"

#include "EnvMapVulkan/EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Game/GameLoop.h"
#include "Game/GameSystems.h"

#include "Input/InputSystem.h"

#include <string>


struct GLFWwindow;


/**
 * The game the player runs (docs/engine/Game_Engine_Runtime_Guide.html, 1.3 and 1.6): the open project's start scene, or
 * the scene given with --scene, seen through its camera entity, over the whole window, run by the game loop with the
 * game's systems. Input comes from the project's input file while the window has the focus. Opening the scene fails: the
 * player closes with exit code 1. Alt+Enter and F11 switch between fullscreen and a maximized window. Escape, unless the
 * game binds it (it may have a pause menu there), first gives a captured mouse back (a click in the window captures it
 * again), then leaves fullscreen. Alt+F4 closes the game. A scene a script loads (Scene.Load, 6.2)
 * replaces the scene's content at the end of the frame, and the game goes on in it.
 */
class PlayerLayer : public H2M::LayerH2M
{
public:
	PlayerLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(H2M::TimestepH2M ts) override;
	virtual void OnEvent(H2M::EventH2M&) override {}
	virtual void OnImGuiRender() override {}

	// The frame, after the update (ApplicationClient::Draw)
	void Draw();

private:
	// Scene.Load: the file's entities in place of the scene's (a file that can't be opened: logged, the game goes on)
	void LoadScene(const std::string& path);

	// Fullscreen and a window: Alt+Enter and F11 switch, Escape gives the mouse back, then leaves fullscreen (unless the game binds it)
	void UpdateFullscreenKeys(GLFWwindow* window);
	void SetFullscreen(GLFWwindow* window, bool fullscreen);

	EnvMapVulkanScene m_Scene;
	std::string m_ScenePath; // the scene's file: Scene.Reload opens it again
	EnvMapVulkanRenderSettings m_RenderSettings;
	Camera m_Camera;
	GameLoop m_GameLoop;
	GameSystems m_Systems;
	InputSystem m_Input;
	bool m_Playing = false; // the scene is open
	bool m_EnterDown = false, m_F11Down = false, m_EscapeDown = false; // last frame's, for the keys' presses
};
