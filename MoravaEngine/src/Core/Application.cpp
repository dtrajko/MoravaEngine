#include "Core/Application.h"

#include "Hazel/Core/Base.h"
#include "Hazel/Renderer/HazelRenderer.h"

#include "Core/Timer.h"

#if defined(HZ_PLATFORM_WINDOWS)
	#define GLFW_EXPOSE_NATIVE_WIN32
	#include <GLFW/glfw3native.h>
	#include <Windows.h>
#endif

#include <imgui.h>


Application* Application::s_Instance = nullptr;

Application::Application()
{
}

void Application::Init(Scene* scene, RendererBasic* renderer) // OnInit() in Hazel
{
	Application::Get(); // make sure the instance is initialized

	s_Instance->m_Scene = scene;
	s_Instance->m_Renderer = renderer;

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		(float)Application::Get()->GetWindow()->GetWidth() / (float)Application::Get()->GetWindow()->GetHeight(),
		Scene::GetSettings().nearPlane, Scene::GetSettings().farPlane);

	RendererBasic::SetProjectionMatrix(projectionMatrix);

	s_Instance->m_Scene->SetCamera();
	s_Instance->m_Scene->SetLightManager();
	s_Instance->m_Scene->SetWaterManager((int)Application::Get()->GetWindow()->GetWidth(), (int)Application::Get()->GetWindow()->GetHeight());

	s_Instance->m_Renderer->Init(s_Instance->m_Scene);

	ImGuiWrapper::Init(Application::Get()->GetWindow());

	float targetFPS = 60.0f;
	float targetUpdateRate = 24.0f;
	Timer timer(targetFPS, targetUpdateRate);
}

// TODO: move game loop from main.cpp here
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

	// OnInit(); // TODO

	// Loop until window closed
	while (s_Instance->m_Running = !Application::Get()->GetWindow()->GetShouldClose())
	{
		Application::Get()->GetWindow()->ProcessEvents(); // Hazel Vulkan: m_Window->ProcessEvents() (currently in Window()->OnUpdate)

		if (!s_Instance->m_Minimized)
		{
			s_Instance->m_Renderer->BeginFrame(); // HazelVulkan: Renderer::BeginFrame();

			s_Instance->m_Scene->Update(Timer::Get()->GetCurrentTimestamp(), Application::Get()->GetWindow()); // TODO deltaTime obsolete

			// Render ImGui on render thread
			ImGuiWrapper::Begin();

			// On Render thread (Hazel Vulkan)
			Application::Get()->GetWindow()->GetRenderContext()->BeginFrame();

			s_Instance->m_Renderer->WaitAndRender(Timer::Get()->GetDeltaTime(), Application::Get()->GetWindow(), s_Instance->m_Scene, RendererBasic::GetProjectionMatrix());

			s_Instance->m_Scene->UpdateImGui(Timer::Get()->GetCurrentTimestamp(), Application::Get()->GetWindow());

			ImGuiWrapper::End();

			// Swap buffers and poll events
			Application::Get()->GetWindow()->SwapBuffers(); // previously Application::Get()->GetWindow()->OnUpdate();
		}
	}
}

// TODO: move cleanup code from main.cpp here
void Application::Cleanup()
{
	ImGuiWrapper::Cleanup();
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

std::string Application::OpenFile(const char* filter) const
{

#if defined(HZ_PLATFORM_WINDOWS)

	OPENFILENAMEA ofn;        // common dialog box structure
	CHAR szFile[260] = { 0 }; // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetHandle());
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		return ofn.lpstrFile;
	}

#endif

	return std::string();
}

std::string Application::SaveFile(const char* filter) const
{

#if defined(HZ_PLATFORM_WINDOWS)

	OPENFILENAMEA ofn;       // common dialog box structure
	CHAR szFile[260] = { 0 };       // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetHandle());
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetSaveFileNameA(&ofn) == TRUE)
	{
		return ofn.lpstrFile;
	}

#endif

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
		auto& caps = Hazel::HazelRenderer::GetCapabilities();

		const char* vendor = "N/A";
		const char* device = "N/A";
		const char* version = "N/A";

		if (Hazel::RendererAPI::Current() == Hazel::RendererAPIType::OpenGL)
		{
			vendor = (const char*)glGetString(GL_VENDOR);
			device = (const char*)glGetString(GL_RENDERER);
			version = (const char*)glGetString(GL_VERSION);
		}

		ImGui::Text("Vendor: %s", vendor);
		ImGui::Text("Device: %s", device);
		ImGui::Text("Version: %s", version);

		ImGui::Text("Frame Time: %.2fms\n", Timer::Get()->GetDeltaTime() * 1000.0f);
	}
	ImGui::End();
}

const char* Application::GetConfigurationName()
{
#if defined(HZ_DEBUG)
	return "Debug";
#elif defined(HZ_RELEASE)
	return "Release";
#elif defined(HZ_DIST)
	return "Dist";
#else
	// #error Undefined configuration?
	return "N/A";
#endif
}

const char* Application::GetPlatformName()
{
#if defined(HZ_PLATFORM_WINDOWS)
	return "Windows x64";
#else
	// #error Undefined platform?
	return "N/A";
#endif
}
