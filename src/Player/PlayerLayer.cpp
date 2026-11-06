#include "PlayerLayer.h"

#include "PlayerProduct.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include "EnvMapVulkan/EnvMapVulkanQuality.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"

#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"

#include "Project/Project.h"

#include "Scripting/ScriptHost.h"

#include "UI/DebugOverlay.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>


PlayerLayer::PlayerLayer()
	: H2M::LayerH2M("PlayerLayer")
{
}

void PlayerLayer::OnAttach()
{
	// The .NET runtime, started by now, has set its own filter for crashes
	if (Project::GetOpen() && Project::GetOpen()->Shipped)
	{
		PlayerProduct::InstallCrashHandler(*Project::GetOpen());
	}

	// --scene, else the project's start scene
	std::string scenePath = Application::Get()->GetCommandLine().ScenePath;
	if (scenePath.empty() && Project::GetOpen())
	{
		scenePath = ProjectPaths::Resolve(Project::GetOpen()->StartScene);
		if (scenePath.empty())
		{
			PlayerProduct::ShowFatalMessage("No scene to play: the project '" + Project::GetOpen()->Name + "' has no start scene.");
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
		PlayerProduct::ShowFatalMessage("The scene '" + scenePath + "' could not be opened (" + error + "). Installing the game again may help.");
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
	if (Project::GetOpen() && Project::GetOpen()->Shipped && !Project::GetOpen()->ScriptAssembly.empty() && !ScriptHost::IsGameAssemblyLoaded())
	{
		PlayerProduct::ShowFatalMessage("The game's scripts could not be started. Please install the game again.");
		m_Playing = false;
		Application::Get()->Close(1);
		return;
	}
	m_Input.SetMap(GameSystems::LoadInputMap());
	m_Systems.SetSceneInfo(name, m_ScenePath);
	// What the game remembered from its last run, in the user's folder: the player's settings and the game's (the C# Settings
	// class), and its progress (SaveData). Without a project: for this run only
	const Project* project = Project::GetOpen();
	m_Systems.OpenSettings(project ? GameSettings::GetUserFile(*project, GameSettings::SettingsFile) : std::string(),
		project ? GameSettings::GetUserFile(*project, GameSettings::SaveDataFile) : std::string());
	const PlayerSettings& player = m_Systems.GetSettings().GetPlayer();
	m_Resolution = player.Resolution.value_or(PlayerResolution());
	GLFWwindow* window = Application::Get()->GetWindow()->GetHandle();
	if (m_Resolution.Width > 0 && !glfwGetWindowMonitor(window))
	{
		PlaceWindow(window); // the player's window size, fitted to the monitor
	}
	StartQuality();
	UpdateDisplayState(Application::Get()->GetWindow()->GetHandle()); // the scripts' OnCreate may ask
	m_Systems.Begin(&m_Scene, &m_Input);
	m_Playing = true;
}

void PlayerLayer::SetQuality(QualityTier tier, const char* why)
{
	if (tier == QualityTier::Auto)
	{
		const QualityGPU gpu = EnvMapVulkanRenderer::GetQualityGPU();
		tier = ChooseQualityTier(gpu);
		Log::GetLogger()->info("Quality: {0}, {1} (chosen for this GPU: {2}, {3} ray tracing): {4}", GetQualityTierName(tier), why,
			gpu.Discrete ? "a graphics card" : "in the processor", gpu.RayTracing ? "with" : "without", EnvMapVulkanQuality::Describe(tier));
	}
	else
	{
		Log::GetLogger()->info("Quality: {0}, {1}: {2}", GetQualityTierName(tier), why, EnvMapVulkanQuality::Describe(tier));
	}
	EnvMapVulkanRenderer::SetQualityTier(tier);
}

void PlayerLayer::StartQuality()
{
	const std::string& option = Application::Get()->GetCommandLine().Quality;
	QualityTier tier = QualityTier::Auto;
	if (!option.empty())
	{
		if (FindQualityTier(option, tier))
		{
			SetQuality(tier, "as --quality asks");
			return;
		}
		Log::GetLogger()->warn("--quality {0}: none of Low, Medium, High, Ultra, Auto", option);
	}
	if (const std::optional<QualityTier>& chosen = m_Systems.GetSettings().GetPlayer().Quality)
	{
		SetQuality(*chosen, "the player's choice");
		return;
	}
	const Project* project = Project::GetOpen();
	SetQuality(project ? project->Build.DefaultQuality : QualityTier::Auto, "the project's default");
}

// The monitor's sizes for fullscreen, largest first, each once: those of its own shape (a 16:9 monitor's 16:9 sizes; the
// others would be stretched or shown with black bars), at least 1024 wide
static std::vector<PlayerResolution> GetMonitorResolutions(GLFWmonitor* monitor)
{
	std::vector<PlayerResolution> resolutions;
	int count = 0;
	const GLFWvidmode* modes = monitor ? glfwGetVideoModes(monitor, &count) : nullptr;
	const GLFWvidmode* own = monitor ? glfwGetVideoMode(monitor) : nullptr;
	const float aspect = own && own->height > 0 ? (float)own->width / (float)own->height : 0.0f;
	for (int i = 0; i < count; i++)
	{
		const PlayerResolution resolution{ (uint32_t)modes[i].width, (uint32_t)modes[i].height };
		const bool shape = aspect <= 0.0f || std::abs((float)modes[i].width / (float)std::max(modes[i].height, 1) - aspect) < 0.02f;
		if (shape && modes[i].width >= 1024 && std::find(resolutions.begin(), resolutions.end(), resolution) == resolutions.end())
		{
			resolutions.push_back(resolution);
		}
	}
	if (resolutions.empty() && own)
	{
		resolutions.push_back(PlayerResolution{ (uint32_t)own->width, (uint32_t)own->height });
	}
	std::sort(resolutions.begin(), resolutions.end(), [](const PlayerResolution& a, const PlayerResolution& b)
	{
		return a.Width != b.Width ? a.Width > b.Width : a.Height > b.Height;
	});
	return resolutions;
}

// The monitor the window is on: its fullscreen monitor, else the one its center is on (the primary when none)
static GLFWmonitor* FindWindowMonitor(GLFWwindow* window)
{
	if (GLFWmonitor* monitor = glfwGetWindowMonitor(window))
	{
		return monitor;
	}
	int x = 0, y = 0, width = 0, height = 0;
	glfwGetWindowPos(window, &x, &y);
	glfwGetWindowSize(window, &width, &height);
	int count = 0;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	for (int i = 0; i < count; i++)
	{
		int mx = 0, my = 0;
		glfwGetMonitorPos(monitors[i], &mx, &my);
		const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
		const int cx = x + width / 2, cy = y + height / 2;
		if (mode && cx >= mx && cx < mx + mode->width && cy >= my && cy < my + mode->height)
		{
			return monitors[i];
		}
	}
	return glfwGetPrimaryMonitor();
}

// The largest of the sizes (largest first) no larger than the one wanted; none: the smallest
static PlayerResolution FitResolution(const std::vector<PlayerResolution>& sizes, const PlayerResolution& wanted)
{
	for (const PlayerResolution& size : sizes)
	{
		if (size.Width <= wanted.Width && size.Height <= wanted.Height)
		{
			return size;
		}
	}
	return sizes.empty() ? wanted : sizes.back();
}

std::vector<PlayerResolution> PlayerLayer::GetWindowResolutions(GLFWmonitor* monitor) const
{
	std::vector<PlayerResolution> fitting;
	if (!monitor)
	{
		return fitting;
	}
	int x = 0, y = 0, width = 0, height = 0;
	glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
	for (const PlayerResolution& size : GetMonitorResolutions(monitor))
	{
		if ((int)size.Width + m_WindowFrame.x + m_WindowFrame.z <= width && (int)size.Height + m_WindowFrame.y + m_WindowFrame.w <= height)
		{
			fitting.push_back(size);
		}
	}
	return fitting;
}

void PlayerLayer::PlaceWindow(GLFWwindow* window)
{
	GLFWmonitor* monitor = glfwGetWindowMonitor(window);
	if (!monitor)
	{
		monitor = FindWindowMonitor(window);
	}
	const Project* project = Project::GetOpen();
	PlayerResolution size{ project ? project->Window.Width : 1280u, project ? project->Window.Height : 720u };
	if (m_Resolution.Width > 0)
	{
		size = FitResolution(GetWindowResolutions(monitor), m_Resolution);
	}
	if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE)
	{
		glfwRestoreWindow(window);
	}
	// Centered in the work area (the screen without the taskbar), its title bar inside it
	int wx = 0, wy = 0, ww = 0, wh = 0;
	if (monitor)
	{
		glfwGetMonitorWorkarea(monitor, &wx, &wy, &ww, &wh);
	}
	const int outerWidth = (int)size.Width + m_WindowFrame.x + m_WindowFrame.z;
	const int outerHeight = (int)size.Height + m_WindowFrame.y + m_WindowFrame.w;
	const int x = monitor ? wx + std::max(0, (ww - outerWidth) / 2) + m_WindowFrame.x : 100;
	const int y = monitor ? wy + std::max(0, (wh - outerHeight) / 2) + m_WindowFrame.y : 100;
	glfwSetWindowMonitor(window, nullptr, x, y, (int)size.Width, (int)size.Height, GLFW_DONT_CARE);
	Log::GetLogger()->info("Player: a window, {0}x{1}; Alt+Enter or F11: fullscreen", size.Width, size.Height);
}

void PlayerLayer::UpdateDisplayState(GLFWwindow* window)
{
	GLFWmonitor* monitor = FindWindowMonitor(window);
	const bool fullscreen = glfwGetWindowMonitor(window) != nullptr;
	bool frameChanged = false;
	if (!fullscreen)
	{
		glm::ivec4 frame(0);
		glfwGetWindowFrameSize(window, &frame.x, &frame.y, &frame.z, &frame.w);
		frameChanged = frame.y > 0 && frame != m_WindowFrame;
		if (frameChanged)
		{
			m_WindowFrame = frame;
		}
	}
	if (monitor != m_ResolutionsMonitor || fullscreen != m_ResolutionsFullscreen || frameChanged)
	{
		m_ResolutionsMonitor = monitor;
		m_ResolutionsFullscreen = fullscreen;
		m_Resolutions = fullscreen ? GetMonitorResolutions(monitor) : GetWindowResolutions(monitor);
	}
	// The size in use: fullscreen's video mode; a window's own size when it is one of the list, else the player's choice as
	// the window would have it
	PlayerResolution resolution = m_Resolution;
	const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
	if (fullscreen && mode)
	{
		resolution = PlayerResolution{ (uint32_t)mode->width, (uint32_t)mode->height };
	}
	else if (!fullscreen)
	{
		int width = 0, height = 0;
		glfwGetWindowSize(window, &width, &height);
		const PlayerResolution own{ (uint32_t)std::max(width, 0), (uint32_t)std::max(height, 0) };
		resolution = std::find(m_Resolutions.begin(), m_Resolutions.end(), own) != m_Resolutions.end() || resolution.Width == 0 ? own :
			FitResolution(m_Resolutions, resolution);
	}
	m_Systems.SetDisplayState(m_Resolutions, resolution, Application::Get()->GetWindow()->IsVSync(), EnvMapVulkanRenderer::GetQualityTier(),
		EnvMapVulkanRenderer::IsRayTracingSupported());
}

void PlayerLayer::ApplyPlayerRequests(GLFWwindow* window)
{
	PlayerResolution resolution;
	if (m_Systems.TakeResolutionRequest(resolution) && resolution != m_Resolution)
	{
		m_Resolution = resolution;
		if (glfwGetWindowMonitor(window))
		{
			SetFullscreen(window, true); // at the new size now
		}
		else
		{
			PlaceWindow(window); // the window at the new size
		}
	}
	bool vsync = false;
	if (m_Systems.TakeVSyncRequest(vsync) && vsync != Application::Get()->GetWindow()->IsVSync())
	{
		Application::Get()->GetWindow()->SetVSync(vsync);
		Log::GetLogger()->info("Player: VSync {0}", vsync ? "on" : "off");
	}
	QualityTier tier = QualityTier::Auto;
	if (m_Systems.TakeQualityRequest(tier))
	{
		SetQuality(tier, tier == QualityTier::Auto ? "the player chose automatic" : "the player's choice");
	}
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
	if (Application::Get()->GetCommandLine().CrashTest == "frame")
	{
		PlayerProduct::Crash("frame");
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
	UpdateDisplayState(window);
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
	ApplyPlayerRequests(window);
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
	m_Systems.EditSettings().EditPlayer().Fullscreen = fullscreen; // the player's choice (the keys', a Settings menu's)
	if (fullscreen)
	{
		// On the monitor the window is on, at the player's resolution when the monitor has it (the highest refresh rate of
		// that size), else at the monitor's own
		GLFWmonitor* target = FindWindowMonitor(window);
		const GLFWvidmode* mode = glfwGetVideoMode(target);
		int count = 0;
		const GLFWvidmode* modes = m_Resolution.Width > 0 ? glfwGetVideoModes(target, &count) : nullptr;
		const GLFWvidmode* chosen = nullptr;
		for (int i = 0; i < count; i++)
		{
			if ((uint32_t)modes[i].width == m_Resolution.Width && (uint32_t)modes[i].height == m_Resolution.Height &&
				(!chosen || modes[i].refreshRate > chosen->refreshRate))
			{
				chosen = &modes[i];
			}
		}
		if (chosen)
		{
			mode = chosen;
		}
		glfwSetWindowMonitor(window, target, 0, 0, mode->width, mode->height, mode->refreshRate);
		Log::GetLogger()->info("Player: fullscreen, {0}x{1} at {2} Hz", mode->width, mode->height, mode->refreshRate);
	}
	else
	{
		// The player's window size (or the project's), centered on the monitor it was on
		PlaceWindow(window);
	}
}

void PlayerLayer::Draw()
{
	EnvMapVulkanRenderer::Draw(&m_Camera);
}
