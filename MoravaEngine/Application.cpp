#include "Application.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>


Application* Application::s_Instance = nullptr;

Application* Application::Get()
{
	if (s_Instance == nullptr)
	{
		s_Instance = new Application();
		s_Instance->m_Window = nullptr;
		s_Instance->m_Scene = nullptr;
		s_Instance->m_Renderer = nullptr;
	}

	return s_Instance;
}

Window* Application::GetWindow()
{
	return m_Window;
}

Scene* Application::GetScene()
{
	return m_Scene;
}

RendererBasic* Application::GetRenderer()
{
	return m_Renderer;
}

void Application::SetWindow(Window* window)
{
	m_Window = window;
}

void Application::SetScene(Scene* scene)
{
	m_Scene = scene;
}

void Application::SetRenderer(RendererBasic* renderer)
{
	m_Renderer = renderer;
}

std::string Application::OpenFile(const std::string& filter) const
{
	OPENFILENAMEA ofn;        // common dialog box structure
	CHAR szFile[260] = { 0 }; // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetHandler());
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		return ofn.lpstrFile;
	}
	return std::string();
}
