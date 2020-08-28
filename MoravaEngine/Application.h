#pragma once

#include "Window.h"
#include "RendererBasic.h"


class Application
{
public:
	static Application* Get();

	// getters
	Window* GetWindow();
	Scene* GetScene();
	RendererBasic* GetRenderer();

	// setters
	void SetWindow(Window* window);
	void SetScene(Scene* scene);
	void SetRenderer(RendererBasic* renderer);

private:
	static Application* s_Instance;
	Window* m_Window;
	Scene* m_Scene;
	RendererBasic* m_Renderer;

};
