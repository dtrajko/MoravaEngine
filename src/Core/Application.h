#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "config.h"

#include "H2M/Core/LayerStackH2M.h"
#include "H2M/Core/Events/ApplicationEventH2M.h"
#include "H2M/Core/Events/EventH2M.h"

#include "Core/Log.h"
#include "Core/Window.h"

#include <memory>


namespace H2M
{
	class ImGuiLayerH2M;
	class RendererAPI_H2M;
}

struct ApplicationSpecification
{
	std::string Name = "MoravaEngine";
	uint32_t WindowWidth = 1280, WindowHeight = 720;
	bool VSync = true;
	std::string WorkingDirectory;
	bool StartMaximized = true;
	bool Resizable = true;
	bool EnableImGui = true;
};

// Options from the command line (parsed in main.cpp)
struct ApplicationCommandLine
{
	std::string ProjectPath;       // --project <file>: a project to open at startup (a .mproject file)
	std::string ScenePath;         // --scene <file>: a scene to open at startup (SceneEnvMapVulkan: an .mscene file)
	float ExitAfterSeconds = 0.0f; // --exit-after <seconds>: closes the window after this time in the main loop (0: never)
	int DebugView = 0;             // --debug-view <number>: the debug view at startup (SceneEnvMapVulkan: the entry of the
	                               // Environment panel's Debug View list, 0 = Off)
	bool NoRayTracing = false;     // --no-ray-tracing: Vulkan without the ray tracing extensions, as on a GPU that has none
	std::string SelectName;        // --select <name>: the entity of this name is selected once the startup scene is open
	bool Play = false;             // --play: the editor starts the game (Play) once the startup scene is open
	bool Simulate = false;         // --simulate: the editor starts Simulate (physics only) once the startup scene is open
	bool SaveScene = false;        // --save-scene: the editor saves the open scene when it closes (after a Play's Stop)
	bool DebugOverlay = false;     // --debug-overlay: the debug overlay (7.4) shown at startup

	static ApplicationCommandLine Parse(int argc, char** argv);
};

class Application;

// What an executable runs on the application, given before Run: the editor's (EditorApplication.cpp) and the player's
// (Player/PlayerMain.cpp). The main loop is the application's, the same for both.
class ApplicationClient
{
public:
	virtual ~ApplicationClient() = default;

	// The older scene class the main loop updates every frame, and its renderer (both nullptr: none)
	virtual SceneProperties CreateScene() = 0;
	// The graphics API's renderer, created by H2M::RendererH2M::Init
	virtual H2M::RendererAPI_H2M* CreateRendererAPI() = 0;
	// The ImGui layer, over the other layers (nullptr: no ImGui)
	virtual H2M::ImGuiLayerH2M* CreateImGuiLayer() { return nullptr; }
	// The layers, once the renderer is initialized
	virtual void PushLayers(Application& application) {}
	// Draws the frame with the graphics API's renderer, after the layers' updates
	virtual void Draw() = 0;
	// The window has a new size (not 0): after the window's swapchain and the framebuffers that follow its size
	virtual void OnWindowResize(uint32_t width, uint32_t height) {}
};

class Application
{
public:
	Application();
	virtual ~Application();

	static Application* Create(const ApplicationSpecification& specification, std::string projectPath);

	static Application* Get();

	void SetClient(std::unique_ptr<ApplicationClient> client) { m_Client = std::move(client); }
	ApplicationClient* GetClient() const { return m_Client.get(); }

	// Shaders, models, textures and scenes are loaded relative to the working directory: false (and why, in the log) when
	// it isn't the MoravaEngine/MoravaEngine folder of the repository
	static bool IsWorkingDirectoryValid();

	void InitWindow(WindowSpecification windowSpecification);

	void OnInit();
	void Run();
	void OnShutdown();

	SceneProperties GetSceneProperties() { return m_SceneProperties; }
	void InitializeScene(SceneProperties sceneProperties); // TODO: Initialize Scene and Renderer here

	void OnEvent(H2M::EventH2M& e);

	void PushLayer(H2M::LayerH2M* layer);
	void PushOverlay(H2M::LayerH2M* layer);

	void RenderImGui(); // Hazel Vulkan Week Day 4 1:26
	void ClassifyEvents();

	// getters
	inline Window* GetWindow() { return m_Window; }
	inline Scene* GetScene() { return m_Scene; }
	inline RendererBasic* GetRenderer() { return m_Renderer; }

	// setters
	inline void SetScene(Scene* scene) { m_Scene = scene; }
	inline void SetRenderer(RendererBasic* renderer) { m_Renderer = renderer; }

	void SetCommandLine(const ApplicationCommandLine& commandLine) { m_CommandLine = commandLine; }
	const ApplicationCommandLine& GetCommandLine() const { return m_CommandLine; }

	// Closes the window after this frame, as if it was closed, and sets the exit code main returns (not 0: the run failed)
	void Close(int exitCode = 0);
	int GetExitCode() const { return m_ExitCode; }

	// The file dialogs of the older scenes (new code: FileDialogs, which these call). filter: a Windows filter string.
	// initialDirectory (optional): the folder the dialog starts in
	std::wstring OpenFile(const wchar_t* filter = L"All\0*.*\0", const wchar_t* initialDirectory = nullptr) const;
	// defaultExtension (e.g. L"mscene", no dot): added to a typed file name without an extension
	std::wstring SaveFile(const wchar_t* filter = L"All\0*.*\0", const wchar_t* defaultExtension = nullptr, const wchar_t* initialDirectory = nullptr) const;

	static const char* GetConfigurationName();
	static const char* GetPlatformName();
	static const char* GetOSName(); // Windows, Linux or macOS

	void CaptureScreenshot(const std::string& filePath);

private:
	bool OnWindowClose(H2M::WindowCloseEventH2M& e);
	bool OnWindowResize(H2M::WindowResizeEventH2M& e);

private:
	static Application* s_Instance;

	ApplicationSpecification m_Specification;
	ApplicationCommandLine m_CommandLine;
	int m_ExitCode = 0;
	std::unique_ptr<ApplicationClient> m_Client;
	std::string m_ProjectPath;

	Scene* m_Scene = nullptr;
	RendererBasic* m_Renderer;

	// Hazel properties
	Window* m_Window;
	bool m_Running = true;
	bool m_Minimized = false;
	H2M::LayerStackH2M m_LayerStack;

	H2M::ImGuiLayerH2M* m_ImGuiLayer = nullptr;
	bool m_EnableImGui = false; // the client has an ImGui layer (ApplicationClient::CreateImGuiLayer)

	float m_TimeStep = 0; // the last frame's time, in seconds, the layers' OnUpdate gets

	SceneProperties m_SceneProperties;

};
