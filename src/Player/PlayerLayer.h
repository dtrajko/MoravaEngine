#pragma once

#include "H2M/Core/LayerH2M.h"

#include "Camera/Camera.h"

#include "EnvMapVulkan/EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Game/GameLoop.h"
#include "Game/GameSystems.h"

#include "Input/InputSystem.h"

#include <glm/glm.hpp>

#include <chrono>
#include <string>
#include <vector>


struct GLFWmonitor;
struct GLFWwindow;


/**
 * The game the player runs (docs/engine/Game_Engine_Runtime_Guide.html, 1.3 and 1.6): the open project's start scene, or
 * the scene given with --scene, seen through its camera entity, over the whole window, run by the game loop with the
 * game's systems. Input comes from the project's input file while the window has the focus. Opening the scene fails: the
 * player closes with exit code 1. Alt+Enter and F11 switch between fullscreen and a maximized window. Escape, unless the
 * game binds it (it may have a pause menu there), first gives a captured mouse back (a click in the window captures it
 * again), then leaves fullscreen. F3 shows and hides the debug overlay (7.4), whatever the game binds. Alt+F4 closes the game. A scene a script loads (Scene.Load, 6.2)
 * replaces the scene's content at the end of the frame, and the game goes on in it.
 *
 * The player's settings (11.4, PlayerSettings) are in the user's folder: the window opens as they say (PlayerMain), the
 * quality tier, the volumes and the bindings are put in place at the start, and what the game's settings menu changes is
 * put in place after the frame and kept (fullscreen by the keys too).
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
	// F3, and the overlay's numbers to the renderer
	void UpdateDebugOverlay(GLFWwindow* window);

	// The player's settings (11.4): the quality tier at the start (--quality, the player's choice, the project's default, the
	// GPU's), and what the game asked for in the frame (the window, the quality), put in place after it
	void StartQuality();
	void SetQuality(QualityTier tier, const char* why);
	void ApplyPlayerRequests(GLFWwindow* window);
	// The scripts' view of the display: the sizes (fullscreen's, or in a window those that fit), the one in use, VSync, the tier
	void UpdateDisplayState(GLFWwindow* window);
	// The first-frames log: the first seconds of a scene (the game's start, Scene.Load), frame by frame at first, then each
	// second, with the GPU's time and its largest passes, and the log file put on the disk at each line
	// (PlayerProduct::SetDurableLog): when a computer turns off then, the log says how far it got
	void BeginFirstFrames();
	void UpdateFirstFrames();

	// A window's sizes on the monitor: the fullscreen sizes whose window, its frame included, fits in the monitor's work area
	std::vector<PlayerResolution> GetWindowResolutions(GLFWmonitor* monitor) const;
	// Out of fullscreen, or a new size in a window: the player's resolution (the largest that fits, no larger than it; none
	// chosen: the project's window), centered on the monitor's work area, not maximized
	void PlaceWindow(GLFWwindow* window);

	EnvMapVulkanScene m_Scene;
	std::string m_ScenePath; // the scene's file: Scene.Reload opens it again
	std::string m_SceneName;
	EnvMapVulkanRenderSettings m_RenderSettings;
	Camera m_Camera;
	GameLoop m_GameLoop;
	GameSystems m_Systems;
	InputSystem m_Input;
	bool m_Playing = false; // the scene is open
	bool m_EnterDown = false, m_F11Down = false, m_EscapeDown = false, m_F3Down = false, m_F1Down = false; // last frame's, for the keys' presses
	bool m_DebugOverlay = false;
	PlayerResolution m_Resolution; // the player's choice: fullscreen's display mode, a window's size (0 x 0: none chosen)
	std::vector<PlayerResolution> m_Resolutions;
	GLFWmonitor* m_ResolutionsMonitor = nullptr; // the monitor m_Resolutions are of
	bool m_ResolutionsFullscreen = false;        // and for which: fullscreen's, or a window's
	glm::ivec4 m_WindowFrame = glm::ivec4(8, 31, 8, 8); // the window's frame (left, top, right, bottom), as last seen in a window
	bool m_FirstFramesOn = false;
	std::chrono::steady_clock::time_point m_FirstFramesStart;
	uint32_t m_FirstFramesUpdates = 0; // updates since the scene started (the frames before them were presented)
	int m_FirstFramesSecond = 0;       // the last whole second logged
};
