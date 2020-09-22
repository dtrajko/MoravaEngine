#pragma once

#include "Window.h"
#include "RendererBasic.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"
#include "Log.h"


class Application
{
public:
	static Application* Get();
	static void Run();

	// getters
	inline Window* GetWindow() { return m_Window; }
	inline Scene* GetScene() { return m_Scene; }
	inline RendererBasic* GetRenderer() { return m_Renderer; }

	// setters
	inline void SetWindow(Window* window) { m_Window = window; }
	inline void SetScene(Scene* scene) { m_Scene = scene; }
	inline void SetRenderer(RendererBasic* renderer) { m_Renderer = renderer; }

	std::string OpenFile(const std::string& filter) const;

private:
	static Application* s_Instance;
	Window* m_Window;
	Scene* m_Scene;
	RendererBasic* m_Renderer;

};
