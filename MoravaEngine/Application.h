#pragma once

#include "Window.h"
#include "RendererBasic.h"

#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Core/LayerStack.h"

#include "Log.h"


class Application
{
public:
	Application();
	virtual ~Application();

	static Application* Get();

	void InitWindow(const WindowProps& props);

	void Run();

	void OnEvent(Event& e);

	// getters
	inline Window* GetWindow() { return m_Window; }
	inline Scene* GetScene() { return m_Scene; }
	inline RendererBasic* GetRenderer() { return m_Renderer; }

	// setters
	inline void SetScene(Scene* scene) { m_Scene = scene; }
	inline void SetRenderer(RendererBasic* renderer) { m_Renderer = renderer; }

	std::string OpenFile(const std::string& filter) const;

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

	// Key cooldown time (emulate onKeyReleased)
	EventCooldown m_KeyPressCooldown = { 0.0f, 0.2f };

};
