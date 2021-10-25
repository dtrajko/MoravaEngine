#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "../../config.h"

#include "Hazel/Core/LayerStack.h"
#include "Hazel/Core/Events/ApplicationEvent.h"
#include "Hazel/Core/Events/Event.h"
#include "Hazel/ImGui/ImGuiLayer.h"
#include "Hazel/Core/Timer.h"

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

	void OnEvent(Event& e);

	void PushLayer(Hazel::Layer* layer);
	void PushOverlay(Hazel::Layer* layer);

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

	const ApplicationSpecification& GetSpecification() const { return m_Specification; }

	Hazel::PerformanceProfiler* GetPerformanceProfiler() { return m_Profiler; }

	Hazel::ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

	void CaptureScreenshot(const std::string& filePath);

private:
	bool OnWindowResize(WindowResizeEvent& e);
	bool OnWindowClose(WindowCloseEvent& e);

private:
	Window* m_Window;
	ApplicationSpecification m_Specification;
	std::string m_ProjectPath;

	Scene* m_Scene;
	RendererBasic* m_Renderer;

	// Hazel properties
	bool m_Running = true;
	bool m_Minimized = false;
	Hazel::LayerStack m_LayerStack;
	Hazel::ImGuiLayer* m_ImGuiLayer;
	float m_TimeStep = 0;
	Hazel::PerformanceProfiler* m_Profiler = nullptr; // TODO: Should be null in Dist
	bool m_ShowStats = true;

	float m_LastFrameTime = 0;

	bool m_EnableImGui = true;

	static Application* s_Instance;
};
