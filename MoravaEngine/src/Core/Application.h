#pragma once

#include "Hazel/Core/LayerStack.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/Event.h"

#include "Core/Log.h"
#include "Core/Window.h"
#include "Renderer/RendererBasic.h"


class Application
{
public:
	Application();
	virtual ~Application();

	static Application* Get();

	void InitWindow(const WindowProps& props);

	static void Run();

	void OnEvent(Event& e);

	// getters
	inline Window* GetWindow() { return m_Window; }
	inline Scene* GetScene() { return m_Scene; }
	inline RendererBasic* GetRenderer() { return m_Renderer; }

	// setters
	inline void SetScene(Scene* scene) { m_Scene = scene; }
	inline void SetRenderer(RendererBasic* renderer) { m_Renderer = renderer; }

	std::string OpenFile(const char* filter = "All\0*.*\0") const;
	std::string SaveFile(const char* filter = "All\0*.*\0") const;

	void OnImGuiRender();

	static const char* GetConfigurationName();
	static const char* GetPlatformName();

private:
	bool OnWindowClose(WindowCloseEvent& e);
	bool OnWindowResize(WindowResizeEvent& e);

private:
	static Application* s_Instance;
	Scene* m_Scene;
	RendererBasic* m_Renderer;

	// Hazel properties
	Window* m_Window;
	bool m_Running = true;
	bool m_Minimized = false;
	LayerStack m_LayerStack;

};
