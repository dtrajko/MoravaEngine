#include "PlayerLayer.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"

#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"

#include "Project/Project.h"

#include "UI/DebugOverlay.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <chrono>
#include <string>


PlayerLayer::PlayerLayer()
	: H2M::LayerH2M("PlayerLayer")
{
}

void PlayerLayer::OnAttach()
{
	// --scene, else the project's start scene
	std::string scenePath = Application::Get()->GetCommandLine().ScenePath;
	if (scenePath.empty() && Project::GetOpen())
	{
		scenePath = ProjectPaths::Resolve(Project::GetOpen()->StartScene);
		if (scenePath.empty())
		{
			Log::GetLogger()->error("No scene to play: the project '{0}' has no start scene", Project::GetOpen()->Name);
			Application::Get()->Close(1);
			return;
		}
	}
	if (scenePath.empty())
	{
		Log::GetLogger()->error("No scene to play: start the player with --project <file.mproject> or --scene <file.mscene>");
		Application::Get()->Close(1);
		return;
	}

	std::string name, error;
	if (!EnvMapVulkanSceneLoader::Open(scenePath, m_Scene, name, m_RenderSettings, error))
	{
		Log::GetLogger()->error("Scene '{0}' could not be opened: {1}", scenePath, error);
		Application::Get()->Close(1);
		return;
	}
	if (EnvMapVulkanSceneData::FindPrimaryCamera(m_Scene) == NoEntity)
	{
		Log::GetLogger()->warn("Scene '{0}' has no camera: seen from the origin, looking along -Z", name);
	}
	Log::GetLogger()->info("Playing scene '{0}' from '{1}': {2} entities", name, scenePath, m_Scene.GetEntityCount());

	// The renderer draws it with the scene's render settings and a game's view (no grid, no selection, no debug view)
	EnvMapVulkanRenderer::SetScene(&m_Scene, &m_RenderSettings, nullptr);

	// The project's scripts, then the game: the scripts' instances are made when it starts
	m_ScenePath = ProjectPaths::Normalize(scenePath);
	m_SceneName = name;
	m_DebugOverlay = Application::Get()->GetCommandLine().DebugOverlay;
	if (m_DebugOverlay)
	{
		Log::GetLogger()->info("Debug overlay: shown (F3 hides it)");
	}
	GameSystems::LoadScripts();
	m_Input.SetMap(GameSystems::LoadInputMap());
	m_Systems.SetSceneInfo(name, m_ScenePath);
	// What the game remembered from its last run, in the user's folder (the C# Settings class)
	m_Systems.OpenSettings(Project::GetOpen() ? GameSettings::GetUserFile(*Project::GetOpen()) : std::string());
	m_Systems.Begin(&m_Scene, &m_Input);
	m_Playing = true;
}

void PlayerLayer::OnDetach()
{
	if (m_Playing)
	{
		Log::GetLogger()->info("Game loop: {0} frames, {1} fixed steps, {2:.2f} s of game time", m_GameLoop.GetFrameCount(),
			m_GameLoop.GetFixedStepCount(), m_GameLoop.GetGameTime());
	}

	m_Input.Reset(Application::Get()->GetWindow()->GetHandle()); // the mouse given back
	EnvMapVulkanRenderer::SetDebugOverlay(false);
	m_Systems.Begin(nullptr, nullptr);

	// The application waited for the device: no frame in flight draws the scene's models any more
	EnvMapVulkanRenderer::SetScene(nullptr, nullptr, nullptr);
	m_Scene.Clear();
}

void PlayerLayer::OnUpdate(H2M::TimestepH2M ts)
{
	if (!m_Playing)
	{
		return; // the scene could not be opened: the player is closing
	}

	// Input (the window is the game: it has input while it has the focus), the game, then the camera where the game put the
	// camera entity
	GLFWwindow* window = Application::Get()->GetWindow()->GetHandle();
	UpdateFullscreenKeys(window);
	const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
	H2M::VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
	const glm::vec2 viewSize((float)swapChain.GetWidth(), (float)swapChain.GetHeight());

	// The game UI gets the mouse first: the buttons it holds are released for the game's actions
	UIPointer pointer;
	if (focused && !m_Input.IsMouseCaptured())
	{
		// The cursor is in the window's units, the view in the framebuffer's pixels (more of them on a Retina display)
		double x = 0.0, y = 0.0;
		int windowWidth = 0, windowHeight = 0;
		glfwGetCursorPos(window, &x, &y);
		glfwGetWindowSize(window, &windowWidth, &windowHeight);
		if (windowWidth > 0 && windowHeight > 0)
		{
			pointer.Position = glm::vec2((float)x, (float)y) * viewSize / glm::vec2((float)windowWidth, (float)windowHeight);
			pointer.Inside = pointer.Position.x >= 0.0f && pointer.Position.y >= 0.0f && pointer.Position.x < viewSize.x &&
				pointer.Position.y < viewSize.y;
		}
	}
	pointer.Buttons = InputSystem::ReadMouseButtons(window);
	m_Input.SetBlockedMouseButtons(m_Systems.UpdateUIPointer(pointer, viewSize));
	m_Input.Update(window, focused, ts.GetSeconds());
	m_Systems.SetFullscreenState(glfwGetWindowMonitor(window) != nullptr);
	m_GameLoop.Frame(ts.GetSeconds(), &m_Systems);
	std::string load;
	if (m_Systems.TakeSceneLoad(load))
	{
		LoadScene(load.empty() ? m_ScenePath : load);
	}
	bool fullscreen = false;
	if (m_Systems.TakeFullscreenRequest(fullscreen) && fullscreen != (glfwGetWindowMonitor(window) != nullptr))
	{
		SetFullscreen(window, fullscreen); // Application.Fullscreen (a Settings menu)
	}
	if (m_Systems.TakeQuit())
	{
		Application::Get()->GetWindow()->SetShouldClose(true); // as if the window was closed: this frame is the last
	}

	// The view fills the window: the camera's aspect ratio follows it (the swapchain's size, in pixels)
	EnvMapVulkanRenderer::SetViewportSize(swapChain.GetWidth(), swapChain.GetHeight());

	// The view of the primary game camera (a script may have switched it), with its lens
	const EnvMapVulkanEntityID cameraEntity = EnvMapVulkanSceneData::FindPrimaryCamera(m_Scene);
	if (cameraEntity != NoEntity)
	{
		EnvMapVulkanSceneData::PlaceGameCamera(m_Camera, m_Scene, cameraEntity);
	}
	UpdateDebugOverlay(window);
}

void PlayerLayer::UpdateDebugOverlay(GLFWwindow* window)
{
	const bool f3 = glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS;
	if (f3 && !m_F3Down)
	{
		m_DebugOverlay = !m_DebugOverlay;
		Log::GetLogger()->info(m_DebugOverlay ? "Debug overlay: shown (F3 hides it)" : "Debug overlay: hidden");
	}
	m_F3Down = f3;
	if (!m_DebugOverlay)
	{
		EnvMapVulkanRenderer::SetDebugOverlay(false);
		return;
	}
	// Pixels per point of the display the window is on (2 on a Retina display, 1.5 at 150% scaling on Windows)
	float scaleX = 1.0f, scaleY = 1.0f;
	glfwGetWindowContentScale(window, &scaleX, &scaleY);
	const DebugOverlayGame game = DebugOverlayGame::Gather(m_GameLoop, m_Systems, m_Scene, m_SceneName);
	EnvMapVulkanRenderer::SetDebugOverlay(true, &game, scaleY);
}

void PlayerLayer::LoadScene(const std::string& path)
{
	const auto start = std::chrono::steady_clock::now();
	EnvMapVulkanRenderer::WaitIdle(); // the scene's models may still be drawn by frames in flight
	std::string name, error;
	EnvMapVulkanRenderSettings settings;
	if (!EnvMapVulkanSceneLoader::Open(path, m_Scene, name, settings, error))
	{
		Log::GetLogger()->error("Scene.Load: '{0}' could not be opened: {1} (the game goes on in this scene)", ProjectPaths::ToStored(path), error);
		return;
	}
	m_RenderSettings = settings;
	m_ScenePath = ProjectPaths::Normalize(path);
	m_SceneName = name;
	EnvMapVulkanRenderer::OnSceneReplaced(); // what the probes gathered belongs to the scene that went
	m_Systems.SetSceneInfo(name, m_ScenePath);
	m_Systems.Restart();
	Log::GetLogger()->info("Scene.Load: scene '{0}' from '{1}': {2} entities, {3:.0f} ms", name, ProjectPaths::ToStored(m_ScenePath),
		m_Scene.GetEntityCount(), std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
}

// A key's press: down now, up in the last frame
static bool WasPressed(GLFWwindow* window, int key, bool& wasDown)
{
	const bool down = glfwGetKey(window, key) == GLFW_PRESS;
	const bool pressed = down && !wasDown;
	wasDown = down;
	return pressed;
}

void PlayerLayer::UpdateFullscreenKeys(GLFWwindow* window)
{
	const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;
	const bool alt = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
	const bool enter = WasPressed(window, GLFW_KEY_ENTER, m_EnterDown);
	const bool f11 = WasPressed(window, GLFW_KEY_F11, m_F11Down);
	const bool escape = WasPressed(window, GLFW_KEY_ESCAPE, m_EscapeDown);
	if ((alt && enter) || f11)
	{
		SetFullscreen(window, !fullscreen);
	}
	else if (escape && !m_Input.GetMap().BindsKey(GLFW_KEY_ESCAPE))
	{
		// First the mouse back to the user (a click in the window gives it to the game again), then out of fullscreen
		if (m_Input.IsMouseCaptured())
		{
			m_Input.ReleaseMouseUntilClick();
		}
		else if (fullscreen)
		{
			SetFullscreen(window, false);
		}
	}
}

void PlayerLayer::SetFullscreen(GLFWwindow* window, bool fullscreen)
{
	if (fullscreen)
	{
		// On the monitor the window is on (its center), at the monitor's own resolution
		int x = 0, y = 0, width = 0, height = 0;
		glfwGetWindowPos(window, &x, &y);
		glfwGetWindowSize(window, &width, &height);
		int count = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
		GLFWmonitor* target = glfwGetPrimaryMonitor();
		for (int i = 0; i < count; i++)
		{
			int mx = 0, my = 0;
			glfwGetMonitorPos(monitors[i], &mx, &my);
			const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
			const int cx = x + width / 2, cy = y + height / 2;
			if (mode && cx >= mx && cx < mx + mode->width && cy >= my && cy < my + mode->height)
			{
				target = monitors[i];
			}
		}
		const GLFWvidmode* mode = glfwGetVideoMode(target);
		glfwSetWindowMonitor(window, target, 0, 0, mode->width, mode->height, mode->refreshRate);
		Log::GetLogger()->info("Player: fullscreen, {0}x{1}", mode->width, mode->height);
	}
	else
	{
		// The project's window size, centered on the monitor it was on, then maximized (its title bar has the close button)
		GLFWmonitor* monitor = glfwGetWindowMonitor(window);
		const Project* project = Project::GetOpen();
		const int width = project ? (int)project->Window.Width : 1280;
		const int height = project ? (int)project->Window.Height : 720;
		int mx = 0, my = 0;
		const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
		if (monitor)
		{
			glfwGetMonitorPos(monitor, &mx, &my);
		}
		const int x = mode ? mx + (mode->width - width) / 2 : 100;
		const int y = mode ? my + (mode->height - height) / 2 : 100;
		glfwSetWindowMonitor(window, nullptr, x, y, width, height, GLFW_DONT_CARE);
		glfwMaximizeWindow(window);
		Log::GetLogger()->info("Player: a window (maximized); Alt+Enter or F11: fullscreen again");
	}
}

void PlayerLayer::Draw()
{
	EnvMapVulkanRenderer::Draw(&m_Camera);
}
