#include "Application.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Timer.h"
#include "EnvironmentMap.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>

#include <imgui.h>


Application* Application::s_Instance = nullptr;

Application::Application()
{
}

void Application::InitWindow(const WindowProps& props)
{
	m_Window = Window::Create(props);
	m_Window->SetEventCallback(APP_BIND_EVENT_FN(OnEvent));
}

Application::~Application()
{
	delete m_Window;
}

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

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<WindowCloseEvent>(APP_BIND_EVENT_FN(OnWindowClose));
	dispatcher.Dispatch<WindowResizeEvent>(APP_BIND_EVENT_FN(OnWindowResize));

	for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
	{
		(*--it)->OnEvent(e);
		if (e.Handled)
			break;
	}
}

void Application::Run()
{
	WindowResizeEvent e(1280, 720);
	Log::GetLogger()->debug(e);

	if (e.IsInCategory(EventCategoryApplication))
	{
		Log::GetLogger()->debug("Event 'WindowResizeEvent' belongs to category 'EventCategoryApplication'");
	}

	if (e.IsInCategory(EventCategoryInput))
	{
		Log::GetLogger()->debug("Event 'WindowResizeEvent' belongs to category 'EventCategoryInput'");
	}
}

std::string Application::OpenFile(const std::string& filter) const
{
	OPENFILENAMEA ofn;        // common dialog box structure
	CHAR szFile[260] = { 0 }; // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetHandle());
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

bool Application::OnWindowClose(WindowCloseEvent& e)
{
	m_Running = false;
	return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
	if (e.GetWidth() == 0 || e.GetHeight() == 0)
	{
		m_Minimized = true;
		return false;
	}

	m_Minimized = false;

	m_Scene->OnWindowResize(e);

	return false;
}

void Application::OnImGuiRender()
{
	ImGui::Begin("Renderer");
	{
		auto& caps = Hazel::RendererAPI::GetCapabilities();
		ImGui::Text("Vendor: %s", caps.Vendor.c_str());
		ImGui::Text("Renderer: %s", caps.Renderer.c_str());
		ImGui::Text("Version: %s", caps.Version.c_str());
		ImGui::Text("Frame Time: %.2fms\n", Timer::Get()->GetDeltaTime() * 1000.0f);
	}
	ImGui::End();
}
