#include "Application.h"


Application* Application::s_Instance = nullptr;
Window* Application::s_Window = nullptr;

Application* Application::Get()
{
	if (s_Instance == nullptr)
	{
		s_Instance = new Application();
	}

	return s_Instance;
}

Window* Application::GetWindow()
{
	return s_Window;
}

void Application::SetWindow(Window* window)
{
	s_Window = window;
}
