#include "Application.h"

#include "Timer.h"
#include "MousePicker.h"
#include "Input.h"
#include "ImGuiWrapper.h"
#include "Profiler.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>


#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

Application* Application::s_Instance = nullptr;

Application::Application()
{
}

void Application::InitWindow(const WindowProps& props)
{
	m_Window = Window::Create(props);
	m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
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
	dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
	dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

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

	// Loop until window closed
	while (m_Running)
	{
		Timer::Get()->Update();

		m_Scene->GetCameraController()->KeyControl(m_Window->getKeys(), Timer::Get()->GetDeltaTime());
		m_Scene->GetCameraController()->MouseControl(m_Window->getMouseButtons(), m_Window->getXChange(), m_Window->getYChange());
		m_Scene->GetCameraController()->MouseScrollControl(m_Window->getKeys(), Timer::Get()->GetDeltaTime(), m_Window->getXMouseScrollOffset(), m_Window->getYMouseScrollOffset());

		MousePicker::Get()->Update(
			(int)m_Window->GetMouseX(), (int)m_Window->GetMouseY(),
			0, 0, (int)m_Window->GetWidth(), (int)m_Window->GetHeight(),
			RendererBasic::GetProjectionMatrix(), m_Scene->GetCameraController()->CalculateViewMatrix());

		if (Input::IsKeyPressed(Key::F))
		{
			LightManager::spotLights[2].GetBasePL()->Toggle();
		}

		// Toggle wireframe mode
		if (Input::IsKeyPressed(Key::R) && !Input::IsKeyPressed(Key::LeftControl))
		{
			if (Timer::Get()->GetCurrentTimestamp() - m_KeyPressCooldown.lastTime > m_KeyPressCooldown.cooldown)
			{
				m_Scene->SetWireframeEnabled(!m_Scene->IsWireframeEnabled());
				m_KeyPressCooldown.lastTime = Timer::Get()->GetCurrentTimestamp();
			}
		}

		if (m_Scene->IsWireframeEnabled())
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		ImGuiWrapper::Begin();

		{
			Profiler profiler("Scene::Update");
			// if (Timer::Get()->CanUpdate())
			m_Scene->Update(Timer::Get()->GetCurrentTimestamp(), m_Window); // TODO deltaTime obsolete
			m_Scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		{
			Profiler profiler("Renderer::Render");
			// if (Timer::Get()->CanRender())
			m_Renderer->Render(Timer::Get()->GetDeltaTime(), m_Window, m_Scene, RendererBasic::GetProjectionMatrix()); // TODO deltaTime obsolete
			m_Scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		m_Scene->UpdateImGui(Timer::Get()->GetCurrentTimestamp(), m_Window);

		m_Scene->GetProfilerResults()->clear();

		ImGuiWrapper::End();

		glDisable(GL_BLEND);

		// Swap buffers and poll events
		m_Window->OnUpdate();
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
