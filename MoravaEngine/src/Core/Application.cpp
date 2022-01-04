#include "Core/Application.h"

#include "H2M/Core/BaseH2M.h"
#include "H2M/Editor/EditorLayerVulkanH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Platform/Vulkan/VulkanTestLayer.h"
#include "H2M/Project/UserPreferencesH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "EnvMapVulkan/EnvMapVulkanEditorLayer.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"

#include "Core/Timer.h"
#include "Platform/DX11/DX11TestLayer.h"
#include "Platform/DX11/DX11Renderer.h"

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

Application* Application::Create(const ApplicationSpecification& specification, std::string projectPath)
{
	Application::Get()->m_Specification = specification;
	Application::Get()->m_ProjectPath = projectPath;

	Application::Get()->InitWindow(WindowProps(specification.Name, specification.WindowWidth, specification.WindowHeight));

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

void Application::OnInit()
{
	m_SceneProperties = Application::SetSceneProperties();
	InitializeScene(m_SceneProperties);

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

	m_Renderer->Init(m_Scene);

	RendererBasic::InitDebug();

	m_ImGuiLayer = H2M::ImGuiLayerH2M::Create();
	PushOverlay(m_ImGuiLayer);

	H2M::RendererH2M::Init();

	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::Vulkan:
			switch (m_SceneProperties.Name)
			{
				case SceneName::HAZEL_VULKAN:
					PushLayer(new H2M::VulkanTestLayer("VulkanTestLayer")); // to be removed
					break;
				case SceneName::ENV_MAP_VULKAN:
					// PushLayer(new H2M::VulkanTestLayer("VulkanTestLayer")); // to be removed
					PushLayer(new EnvMapVulkanEditorLayer(H2M::RefH2M<H2M::UserPreferencesH2M>::Create())); // Used in SCENE_ENV_MAP_VULKAN
					break;
				default:
					// PushLayer(new H2M::EditorLayerVulkanH2M(H2M::RefH2M<H2M::UserPreferencesH2M>::Create())); // Used in Hazel-dev
					break;
			}
			break;
		case H2M::RendererAPITypeH2M::DX11:
			PushLayer(new DX11TestLayer("DX11TestLayer"));
			break;
	}

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
	// auto& caps = H2M::RendererH2M::GetCapabilities();
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

	// Loop until window closed
	while (m_Running = !m_Window->GetShouldClose())
	{
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

			m_Scene->Update(Timer::Get()->GetCurrentTimestamp(), m_Window);

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

			m_Renderer->WaitAndRender(deltaTime, m_Window, m_Scene, RendererBasic::GetProjectionMatrix());

			m_Scene->UpdateImGui(Timer::Get()->GetCurrentTimestamp(), m_Window);

			switch (H2M::RendererAPI_H2M::Current())
			{
			case H2M::RendererAPITypeH2M::Vulkan:
				// m_Scene->OnRenderEditor(deltaTime, *(H2M::EditorCamera*)m_Scene->GetCamera());
				switch (m_SceneProperties.Name)
				{
					case SceneName::HAZEL_VULKAN:
						H2M::VulkanRendererH2M::Draw(m_Scene->GetCamera()); // replace with m_Scene->OnRenderEditor()
						break;
					case SceneName::ENV_MAP_VULKAN:
						EnvMapVulkanRenderer::Draw(m_Scene->GetCamera()); // replace with m_Scene->OnRenderEditor()
						// TODO EnvMapVulkanRenderer::Render() (?)
						break;
				}
				break;
			case H2M::RendererAPITypeH2M::DX11:
				DX11Renderer::Draw(m_Scene->GetCamera());
				break;
			}

			H2M::RendererH2M::Submit([=]() { m_ImGuiLayer->End(); });

			// Swap buffers and poll events
			m_Window->SwapBuffers();
		}

		float time = Timer::Get()->GetCurrentTimestamp();
		m_TimeStep = time - m_LastFrameTime;
		m_LastFrameTime = time;
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

	m_Scene->OnWindowResize(e);
	m_Window->GetRenderContext()->OnResize(width, height);

	switch (H2M::RendererAPI_H2M::Current())
	{
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
		case H2M::RendererAPITypeH2M::DX11:
		{
			DX11Renderer::OnResize(width, height);
		}
		break;
	}

	return false;
}

bool Application::OnWindowClose(H2M::WindowCloseEventH2M& e)
{
	m_Running = false;
	return true;
}

void Application::OnShutdown()
{
	delete m_ImGuiLayer;

	if (m_Scene != nullptr) delete m_Scene;
	delete m_Renderer;
}

void Application::InitWindow(WindowProps& props)
{
	RendererBasic::AppendRendererInfo(props);

	m_Window = Window::Create(props);
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
	Log::GetLogger()->debug(e);

	if (e.IsInCategory(H2M::EventCategoryApplication))
	{
		Log::GetLogger()->debug("Event 'WindowResizeEvent' belongs to category 'EventCategoryApplication'");
	}

	if (e.IsInCategory(H2M::EventCategoryInput))
	{
		Log::GetLogger()->debug("Event 'WindowResizeEvent' belongs to category 'EventCategoryInput'");
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
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::OpenGL:
		case H2M::RendererAPITypeH2M::Vulkan:
			ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetHandle());
			break;
		case H2M::RendererAPITypeH2M::DX11:
			ofn.hwndOwner = m_Window->GetHWND();
			break;
	}
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

	OPENFILENAMEA ofn;        // common dialog box structure
	CHAR szFile[260] = { 0 }; // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::OpenGL:
		case H2M::RendererAPITypeH2M::Vulkan:
			ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)m_Window->GetHandle());
			break;
		case H2M::RendererAPITypeH2M::DX11:
			ofn.hwndOwner = m_Window->GetHWND();
			break;
	}
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

void Application::OnImGuiRender(bool* p_open)
{
	ImGui::Begin("Renderer", p_open);
	{
		auto& caps = H2M::RendererH2M::GetCapabilities();

		const char* vendor = "N/A";
		const char* device = "N/A";
		const char* version = "N/A";

		if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
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

SceneProperties Application::SetSceneProperties()
{
	SceneProperties sceneProperties;

#if defined(SCENE_COTTAGE)
	sceneProperties.Scene = new SceneCottage();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::COTTAGE;
#elif defined(SCENE_EIFFEL)
	sceneProperties.Scene = new SceneEiffel();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::EIFFEL;
#elif defined(SCENE_SPONZA)
	sceneProperties.Scene = new SceneSponza();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::SPONZA;
#elif defined(SCENE_TERRAIN)
	sceneProperties.Scene = new SceneTerrain();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::TERRAIN;
#elif defined(SCENE_PBR)
	sceneProperties.Scene = new ScenePBR();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererPBR());
	sceneProperties.Name = SceneName::PBR;
#elif defined(SCENE_LEARN_OPENGL)
	sceneProperties.Scene = new SceneJoey();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::LEARN_OPENGL;
#elif defined(SCENE_BULLET)
	sceneProperties.Scene = new SceneBullet();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::BULLET;
#elif defined(SCENE_INSTANCED)
	sceneProperties.Scene = new SceneInstanced();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::INSTANCED;
#elif defined(SCENE_ASTEROIDS)
	sceneProperties.Scene = new SceneAsteroids();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::ASTEROIDS;
#elif defined(SCENE_NANOSUIT)
	sceneProperties.Scene = new SceneNanosuit();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::NANOSUIT;
#elif defined(SCENE_FRAMEBUFFERS)
	sceneProperties.Scene = new SceneFramebuffers();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::FRAMEBUFFERS;
#elif defined(SCENE_CUBEMAPS)
	sceneProperties.Scene = new SceneCubemaps();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::CUBEMAPS;
#elif defined(SCENE_PARTICLES)
	sceneProperties.Scene = new SceneParticles();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::PARTICLES;
#elif defined(SCENE_OMNI_SHADOWS)
	sceneProperties.Scene = new SceneOmniShadows();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererOmniShadows());
	sceneProperties.Name = SceneName::OMNI_SHADOWS;
#elif defined(SCENE_PROCEDURAL_LANDMASS)
	sceneProperties.Scene = new SceneProceduralLandmass();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::PROCEDURAL_LANDMASS;
#elif defined(SCENE_VOXEL_TERRAIN)
	sceneProperties.Scene = new SceneVoxelTerrain();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::VOXEL_TERRAIN;
#elif defined(SCENE_VOXEL_TERRAIN_SL)
	sceneProperties.Scene = new SceneVoxelTerrainSL();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::VOXEL_TERRAIN_SL;
#elif defined(SCENE_MARCHING_CUBES)
	sceneProperties.Scene = new SceneMarchingCubes();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::MARCHING_CUBES;
#elif defined(SCENE_SSAO)
	sceneProperties.Scene = new SceneSSAO();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::SSAO;
#elif defined(SCENE_BLOOM)
	sceneProperties.Scene = new SceneBloom();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::BLOOM;
#elif defined(SCENE_ANIM_PBR)
	sceneProperties.Scene = new SceneAnimPBR();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::ANIM_PBR;
#elif defined(SCENE_DEFERRED)
	sceneProperties.Scene = new SceneDeferred();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::DEFERRED;
#elif defined(SCENE_DEFERRED_OGL)
	sceneProperties.Scene = new SceneDeferredOGL();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererDeferredOGL());
	sceneProperties.Name = SceneName::DEFERRED_OGL;
#elif defined(SCENE_EDITOR)
	sceneProperties.Scene = new SceneEditor();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererEditor());
	sceneProperties.Name = SceneName::EDITOR;
#elif defined(SCENE_EDITOR_IMGUIZMO)
	sceneProperties.Scene = new SceneEditorImGuizmo();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererEditor());
	sceneProperties.Name = SceneName::EDITOR_IMGUIZMO;
#elif defined(SCENE_HAZEL_ENV_MAP)
	sceneProperties.Scene = new SceneHazelEnvMap();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererECS());
	sceneProperties.Name = SceneName::HAZEL_ENV_MAP;
#elif defined(SCENE_DX11)
	sceneProperties.Scene = new SceneDX11();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::HAZEL_VULKAN;
#elif defined(SCENE_HAZEL_VULKAN)
	sceneProperties.Scene = new SceneHazelVulkan();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::HAZEL_VULKAN;
#elif defined(SCENE_ENV_MAP_VULKAN)
	sceneProperties.Scene = new SceneEnvMapVulkan();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererECS());
	sceneProperties.Name = SceneName::ENV_MAP_VULKAN;
#else
	sceneProperties.Scene = nullptr;
	sceneProperties.Renderer = nullptr;
	sceneProperties.Name = SceneName::NONE;
	throw std::runtime_error("Scene and Renderer could not be loaded!");
#endif

	return sceneProperties;
}
