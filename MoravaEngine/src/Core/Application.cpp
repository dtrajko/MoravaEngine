#include "Core/Application.h"

#include "H2M/Core/BaseH2M.h"
#include "H2M/ImGui/ImGuiLayerH2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#endif
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/FileDialogs.h"
#include "Core/ResourceManager.h"
#include "Core/Timer.h"
#include "Material/MaterialLibrary.h"
#include "Core/Util.h"
#include "Renderer/RendererBasic.h"
#include "Scene/Scene.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>


Application* Application::s_Instance = nullptr;

Application::Application()
{
	// m_EnableImGui = false;
}

Application* Application::Create(const ApplicationSpecification& specification, std::string projectPath)
{
	Application::Get()->m_Specification = specification;
	Application::Get()->m_ProjectPath = projectPath;

	Application::Get()->InitWindow(WindowSpecification{});

	return Application::Get();
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

// Command line options:
//   --scene <file>          open this scene at startup (SceneEnvMapVulkan: an .mscene file, e.g. Scenes/Old_Stove_Corner.mscene)
//   --exit-after <seconds>  close the application after this time in the main loop
//   --debug-view <number>   start with this debug view (SceneEnvMapVulkan: the entry of the Environment panel's Debug View
//                           list, 0 = Off, e.g. 8 = Probe Irradiance Only, 10 = Ambient Occlusion)
//   --no-ray-tracing        Vulkan without the ray tracing extensions, as on a GPU that has none (to check the fallbacks)
//   --select <name>         select the entity of this name in the scene opened with --scene (SceneEnvMapVulkan: a name of
//                           the Scene Hierarchy, e.g. Gladiator.fbx)
//   --play                  the editor starts the game (Play) once the scene is open; with --exit-after it is stopped
//                           at exit, which checks that the edited scene came back unchanged (the player always plays)
ApplicationCommandLine ApplicationCommandLine::Parse(int argc, char** argv)
{
	ApplicationCommandLine commandLine;
	for (int i = 1; i < argc; i++)
	{
		const std::string option = argv[i];
		const bool hasValue = i + 1 < argc;
		if (option == "--scene" && hasValue)
		{
			commandLine.ScenePath = argv[++i];
		}
		else if (option == "--exit-after" && hasValue)
		{
			const char* value = argv[++i];
			char* end = nullptr;
			float seconds = std::strtof(value, &end);
			if (end == value || *end != '\0' || seconds <= 0.0f)
			{
				Log::GetLogger()->warn("Command line: --exit-after needs a number of seconds above 0, got '{0}' (ignored)", value);
			}
			else
			{
				commandLine.ExitAfterSeconds = seconds;
			}
		}
		else if (option == "--debug-view" && hasValue)
		{
			const char* value = argv[++i];
			char* end = nullptr;
			long view = std::strtol(value, &end, 10);
			if (end == value || *end != '\0' || view < 0)
			{
				Log::GetLogger()->warn("Command line: --debug-view needs a number from 0 on, got '{0}' (ignored)", value);
			}
			else
			{
				commandLine.DebugView = (int)view;
			}
		}
		else if (option == "--no-ray-tracing")
		{
			commandLine.NoRayTracing = true;
		}
		else if (option == "--select" && hasValue)
		{
			commandLine.SelectName = argv[++i];
		}
		else if (option == "--play")
		{
			commandLine.Play = true;
		}
		else if (option == "--scene" || option == "--exit-after" || option == "--debug-view" || option == "--select")
		{
			Log::GetLogger()->warn("Command line: {0} needs a value (ignored)", option);
		}
		else
		{
			Log::GetLogger()->warn("Command line: unknown option '{0}' (ignored)", option);
		}
	}
	return commandLine;
}

// Started anywhere else, the first shader comes back empty and the engine fails much later with an unrelated error, so
// this says it at the start
bool Application::IsWorkingDirectoryValid()
{
	std::error_code error;
	if (std::filesystem::is_directory("Resources/Shaders", error))
	{
		return true;
	}
	const std::string folder = std::filesystem::current_path(error).u8string();
	Log::GetLogger()->error("Resources/Shaders not found in the working directory '{0}'.", folder);
	Log::GetLogger()->error("Start MoravaEngine from the MoravaEngine/MoravaEngine folder of the repository.");
	return false;
}

void Application::OnInit()
{
	H2M_CORE_ASSERT(m_Client, "Application::SetClient before Run");
	m_SceneProperties = m_Client->CreateScene();
	InitializeScene(m_SceneProperties);

	if (m_Scene)
	{
		// Projection matrix
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
			(float)Application::Get()->GetWindow()->GetWidth() / (float)Application::Get()->GetWindow()->GetHeight(),
			Scene::GetSettings().nearPlane, Scene::GetSettings().farPlane);

		RendererBasic::SetProjectionMatrix(projectionMatrix);

		m_Scene->SetCamera();
		m_Scene->SetLightManager();
		m_Scene->SetWaterManager(
			(int)Application::Get()->GetWindow()->GetWidth(),
			(int)Application::Get()->GetWindow()->GetHeight());
	}

	if (m_Renderer)
	{
		m_Renderer->Init(m_Scene);
	}

	RendererBasic::InitDebug();

	m_ImGuiLayer = m_Client->CreateImGuiLayer();
	m_EnableImGui = m_ImGuiLayer != nullptr;
	if (m_EnableImGui)
	{
		PushOverlay(m_ImGuiLayer);
	}

	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
	{
#if defined(OPENGL_4_5_PLUS)
		H2M::RendererH2M::Init();
#endif
	}
	else
	{
		H2M::RendererH2M::Init();
	}

	m_Client->PushLayers(*this);

	float targetFPS = 60.0f;
	float targetUpdateRate = 24.0f;
	Timer timer(targetFPS, targetUpdateRate);
}

Application::~Application()
{
	delete m_Window;
}

void Application::PushLayer(H2M::LayerH2M* layer)
{
	m_LayerStack.PushLayer(layer);
	layer->OnAttach();
}

void Application::PushOverlay(H2M::LayerH2M* layer)
{
	m_LayerStack.PushOverlay(layer);
	layer->OnAttach();
}

void Application::RenderImGui()
{
	m_ImGuiLayer->Begin();

	// ImGui::Begin("Renderer");
	// auto caps = H2M::RendererH2M::GetCapabilities();
	// ImGui::Text("Vendor: %s", caps.Vendor.c_str());
	// ImGui::Text("Device: %s", caps.Device.c_str());
	// ImGui::Text("Version: %s", caps.Version.c_str());
	// ImGui::Text("Frame Time: %.2fms\n", Timer::Get()->GetDeltaTime() * 1000.0f);
	// ImGui::End();

	for (H2M::LayerH2M* layer : m_LayerStack)
	{
		layer->OnImGuiRender();
	}

	// m_ImGuiLayer->End();
}

// TODO: move game loop from main.cpp here
void Application::Run()
{
	OnInit();

	// --exit-after: counted from here, after the startup (the shaders may have been compiled)
	const auto loopStart = std::chrono::steady_clock::now();
	auto frameStart = loopStart;

	// Loop until window closed
	while ((m_Running = !m_Window->GetShouldClose()))
	{
		if (m_CommandLine.ExitAfterSeconds > 0.0f &&
			std::chrono::duration<float>(std::chrono::steady_clock::now() - loopStart).count() >= m_CommandLine.ExitAfterSeconds)
		{
			Log::GetLogger()->info("Closing after {0} s (--exit-after)", m_CommandLine.ExitAfterSeconds);
			m_Window->SetShouldClose(true); // as if the window was closed: this frame is the last
		}

		float deltaTime = Timer::Get()->GetDeltaTime(); // can be used as H2M::Timestep

		m_Window->ProcessEvents();

		if (!m_Minimized)
		{
			// H2M::RendererH2M::BeginFrame();
			{
				for (H2M::LayerH2M* layer : m_LayerStack)
				{
					layer->OnUpdate(m_TimeStep);
				}
			}

			if (m_Scene)
			{
				m_Scene->Update(Timer::Get()->GetCurrentTimestamp(), m_Window);
			}

			// Render ImGui on render thread
			Application* app = this;
			if (m_EnableImGui)
			{
				H2M::RendererH2M::Submit([app]() { app->RenderImGui(); });
				// H2M::RendererH2M::Submit([=]() { m_ImGuiLayer->End(); });
			}
			// H2M::RendererH2M::EndFrame();

			// On Render thread
			m_Window->GetRenderContext()->BeginFrame();

			if (m_Renderer)
			{
				m_Renderer->WaitAndRender(deltaTime, m_Window, m_Scene, RendererBasic::GetProjectionMatrix());
			}

			if (m_EnableImGui && m_Scene)
			{
				m_Scene->UpdateImGui(Timer::Get()->GetCurrentTimestamp(), m_Window);
			}

			m_Client->Draw();

			if (m_EnableImGui)
			{
				H2M::RendererH2M::Submit([=]() { m_ImGuiLayer->End(); });
			}

			// Swap buffers and poll events
			m_Window->SwapBuffers();
		}

		// The frame's time, for the layers next frame. Measured here: the Timer is ticked only by an older scene class
		const auto frameEnd = std::chrono::steady_clock::now();
		m_TimeStep = std::chrono::duration<float>(frameEnd - frameStart).count();
		frameStart = frameEnd;
	}

	OnShutdown();
}

void Application::OnEvent(H2M::EventH2M& e)
{
	H2M::EventDispatcherH2M dispatcher(e);
	dispatcher.Dispatch<H2M::WindowCloseEventH2M>(APP_BIND_EVENT_FN(OnWindowClose));
	dispatcher.Dispatch<H2M::WindowResizeEventH2M>(APP_BIND_EVENT_FN(OnWindowResize));

	for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
	{
		(*--it)->OnEvent(e);
		if (e.Handled)
			break;
	}
}

bool Application::OnWindowResize(H2M::WindowResizeEventH2M& e)
{
	int width = e.GetWidth(), height = e.GetHeight();

	if (width == 0 || height == 0)
	{
		m_Minimized = true;
		return false;
	}

	m_Minimized = false;
	H2M::RendererH2M::Submit([=]() { glViewport(0, 0, width, height); });

	if (m_Scene)
	{
		m_Scene->OnWindowResize(e);
	}
	m_Window->GetRenderContext()->OnResize(width, height);

	switch (H2M::RendererAPI_H2M::Current())
	{
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan:
		{
			auto& fbs = H2M::FramebufferPoolH2M::GetGlobal()->GetAll();
			for (auto& fb : fbs)
			{
				const auto& spec = fb->GetSpecification();
				if (spec.Width == 0 || spec.Height == 0)
				{
					fb->Resize((uint32_t)(width * spec.Scale), (uint32_t)(height * spec.Scale));
				}
			}
		}
		break;
#endif
		default: break;
	}

	m_Client->OnWindowResize((uint32_t)width, (uint32_t)height);

	return false;
}

void Application::Close(int exitCode)
{
	m_ExitCode = exitCode;
	m_Window->SetShouldClose(true);
}

bool Application::OnWindowClose(H2M::WindowCloseEventH2M& e)
{
	m_Running = false;
	return true;
}

void Application::OnShutdown()
{
#if defined(GRAPHICS_API_VULKAN)
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan)
	{
		vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); // the last frames may still use what is released below
	}
#endif

	// Layers detach in reverse order. The ImGui layer is left out: deleting it (below) shuts ImGui down, as its OnDetach would
	for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
	{
		if (*it != m_ImGuiLayer)
		{
			(*it)->OnDetach();
		}
	}

	if (m_EnableImGui)
	{
		delete m_ImGuiLayer;
	}

	if (m_Scene != nullptr) delete m_Scene;
	delete m_Renderer;
	MaterialLibrary::Cleanup();
	ResourceManager::Shutdown();

	// The renderer's resources are released here, while main still runs: kept until the static variables are
	// destroyed (after main returns), they would call into a graphics API that may already be torn down.
	// Same condition as RendererH2M::Init in Application::Init.
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
	{
#if defined(OPENGL_4_5_PLUS)
		H2M::RendererH2M::Shutdown();
#endif
	}
	else
	{
		H2M::RendererH2M::Shutdown();
	}
}

void Application::InitWindow(WindowSpecification windowSpecification)
{
	RendererBasic::AppendRendererInfo(windowSpecification);

	m_Window = Window::Create(windowSpecification);
	m_Window->SetEventCallback(APP_BIND_EVENT_FN(Application::OnEvent));
}

void Application::InitializeScene(SceneProperties sceneProperties)
{
	m_Scene = sceneProperties.Scene;
	m_Renderer = sceneProperties.Renderer;
}

// Event handling code extracted and removed from Application::Run(). Currently not in use.
void Application::ClassifyEvents()
{
	H2M::WindowResizeEventH2M e(1280, 720);
	Log::GetLogger()->debug("WindowResizeEvent: {}x{}", e.GetWidth(), e.GetHeight());

	if (e.IsInCategory(H2M::EventCategoryApplication))
	{
		Log::GetLogger()->debug("Event 'WindowResizeEvent' belongs to category 'EventCategoryApplication'");
	}

	if (e.IsInCategory(H2M::EventCategoryInput))
	{
		Log::GetLogger()->debug("Event 'WindowResizeEvent' belongs to category 'EventCategoryInput'");
	}
}

// The dialogs of the older scenes, with a Windows filter string and wide characters: see FileDialogs for new code
std::wstring Application::OpenFile(const wchar_t* filter, const wchar_t* initialDirectory) const
{
	const std::string path = FileDialogs::OpenFile(FileDialogs::FiltersFromWindowsString(filter),
		initialDirectory ? Util::to_str(initialDirectory) : std::string());
	return Util::to_wstr(path);
}

std::wstring Application::SaveFile(const wchar_t* filter, const wchar_t* defaultExtension, const wchar_t* initialDirectory) const
{
	std::vector<FileDialogFilter> filters = FileDialogs::FiltersFromWindowsString(filter);
	if (filters.empty() && defaultExtension && *defaultExtension)
	{
		filters.push_back({ Util::to_str(defaultExtension), Util::to_str(defaultExtension) });
	}
	const std::string path = FileDialogs::SaveFile(filters, initialDirectory ? Util::to_str(initialDirectory) : std::string());
	return Util::to_wstr(path);
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
#elif defined(__APPLE__)
	return "macOS";
#elif defined(HZ_PLATFORM_LINUX)
	return "Linux";
#else
	// #error Undefined platform?
	return "N/A";
#endif
}

const char* Application::GetOSName()
{
#if defined(_WIN32)
	return "Windows";
#elif defined(__APPLE__)
	return "macOS";
#elif defined(__linux__)
	return "Linux";
#else
	return "Unknown OS";
#endif
}

void Application::CaptureScreenshot(const std::string& filePath)
{
	if (H2M::RendererAPI_H2M::Current() != H2M::RendererAPITypeH2M::OpenGL) return;

	int width, height;
	glfwGetFramebufferSize(m_Window->GetHandle(), &width, &height);
	uint8_t* ptr = (uint8_t*)malloc(width * height * 4);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
	stbi_flip_vertically_on_write(1);
	// stbi_write_png(filePath.c_str(), width, height, 4, ptr, 0);
	stbi_write_jpg(filePath.c_str(), width, height, 4, ptr, 60);
	free(ptr);
}
