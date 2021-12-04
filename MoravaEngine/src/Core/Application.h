#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "../../config.h"

#include "H2M/Core/LayerStackH2M.h"
#include "H2M/Core/Events/ApplicationEventH2M.h"
#include "H2M/Core/Events/EventH2M.h"
#include "H2M/ImGui/ImGuiLayerH2M.h"

#include "Core/Log.h"
#include "Core/Window.h"
#include "Renderer/RendererBasic.h"


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

class Application
{
public:
	Application();
	virtual ~Application();

	static Application* Create(const ApplicationSpecification& specification, std::string projectPath);

	static Application* Get();

	void InitWindow(WindowProps& props);

	void OnInit();
	void Run();
	void OnShutdown();

	SceneProperties Application::SetSceneProperties();
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

	std::string OpenFile(const char* filter = "All\0*.*\0") const;
	std::string SaveFile(const char* filter = "All\0*.*\0") const;

	void OnImGuiRender(bool* p_open = (bool*)0);

	static const char* GetConfigurationName();
	static const char* GetPlatformName();

	void CaptureScreenshot(const std::string& filePath);

private:
	bool OnWindowClose(H2M::WindowCloseEventH2M& e);
	bool OnWindowResize(H2M::WindowResizeEventH2M& e);

private:
	static Application* s_Instance;

	ApplicationSpecification m_Specification;
	std::string m_ProjectPath;

	Scene* m_Scene;
	RendererBasic* m_Renderer;

	// Hazel properties
	Window* m_Window;
	bool m_Running = true;
	bool m_Minimized = false;
	H2M::LayerStackH2M m_LayerStack;

	H2M::ImGuiLayerH2M* m_ImGuiLayer;
	bool m_EnableImGui = true;

	float m_TimeStep = 0;
	float m_LastFrameTime = 0;

};
