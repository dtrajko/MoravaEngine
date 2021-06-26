#include "DX11TestLayer.h"

#include "DX11Context.h"
#include "DX11SwapChain.h"
#include "DX11Renderer.h"
#include "DX11Shader.h"
#include "DX11InputSystem.h"

#include "Core/Application.h"


std::shared_ptr<DX11CameraFP> DX11TestLayer::s_Camera;


DX11TestLayer::DX11TestLayer()
{
	s_Camera = std::make_shared<DX11CameraFP>(glm::perspectiveFov(glm::radians(60.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
}

DX11TestLayer::DX11TestLayer(const std::string& name) : Layer(name)
{
	s_Camera = std::make_shared<DX11CameraFP>(glm::perspectiveFov(glm::radians(60.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
}

DX11TestLayer::~DX11TestLayer()
{
}

void DX11TestLayer::OnAttach()
{
	// m_Meshes.push_back(Hazel::Ref<Hazel::HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx"));

	DX11InputSystem::Get()->AddListener(this);

	// Application::Get()->GetWindow()->SetInFocus(false);
	m_ShowMouseCursor = true;
	DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor);
}

void DX11TestLayer::OnDetach()
{
}

void DX11TestLayer::OnUpdate(Hazel::Timestep ts)
{
	bool windowInFocus = Application::Get()->GetWindow()->IsInFocus();
	bool cameraEnabled = windowInFocus && !m_ShowMouseCursor;
	s_Camera->SetEnabled(cameraEnabled);

	// Log::GetLogger()->info("windowInFocus: {0}, m_ShowMouseCursor: {1}, cameraEnabled: {2}", windowInFocus, m_ShowMouseCursor, cameraEnabled);

	DX11InputSystem::Get()->Update();

	s_Camera->OnUpdate(ts);

	s_Camera->SetProjectionMatrix(
		glm::perspectiveFov(glm::radians(45.0f), (float)DX11Renderer::GetViewportWidth(), (float)DX11Renderer::GetViewportHeight(), 0.01f, 1000.0f));

	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	Render(clearColor, s_Camera);
	for (Hazel::Ref<Hazel::HazelMesh> mesh : m_Meshes)
	{
		DX11Renderer::SubmitMesh(mesh);
	}
}

void DX11TestLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
}

void DX11TestLayer::OnEvent(Event& event)
{
	s_Camera->OnEvent(event);

	if (event.GetEventType() == EventType::WindowResize)
	{
		WindowResizeEvent& e = (WindowResizeEvent&)event;
		if (e.GetWidth() != 0 && e.GetHeight() != 0)
		{
			s_Camera->SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());
			s_Camera->SetProjectionMatrix(glm::perspectiveFov(glm::radians(60.0f), (float)e.GetWidth(), (float)e.GetHeight(), 0.1f, 10000.0f));
		}
	}
}

void DX11TestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
}

void DX11TestLayer::OnRender(Window* mainWindow)
{
}

void DX11TestLayer::Render(const glm::vec4& clearColor, std::shared_ptr<DX11CameraFP> camera)
{
	if (!m_Meshes.size()) return;

	auto mesh = m_Meshes[0];
	// HazelRenderer::Submit([=]() mutable
	// {
	// });
	{
		Hazel::Ref<DX11Context> context = Hazel::Ref<DX11Context>(Application::Get()->GetWindow()->GetRenderContext());
		// TODO:	Hazel::Ref<DX11Shader> shader = mesh->GetMeshShader().As<DX11Shader>();
		// TODO:	DX11SwapChain& swapChain = context->GetSwapChain();

		// TODO:	{
		// TODO:		// uniform buffer binding 0 uniform Camera
		// TODO:		void* ubPtr = shader->MapUniformBuffer(0);
		// TODO:		glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)swapChain.GetWidth(), (float)swapChain.GetHeight(), 0.1f, 1000.0f);
		// TODO:		// glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 4.0f)));
		// TODO:		glm::mat4 viewProj = proj * camera.GetViewMatrix();  // Runtime camera
		// TODO:		// glm::mat4 viewProj = m_Camera.GetViewProjection(); // Editor camera
		// TODO:		memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
		// TODO:		shader->UnmapUniformBuffer(0);
		// TODO:	}
	}
}

void DX11TestLayer::OnKeyDown(int key)
{
}

void DX11TestLayer::OnKeyUp(int key)
{
	if (key == VK_ESCAPE)
	{
		m_ShowMouseCursor = true;

		// Application::Get()->GetWindow()->SetInFocus(false);
		DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor);
	}
}

void DX11TestLayer::OnMouseMove(const glm::vec2& deltaMousePos)
{
}

void DX11TestLayer::OnLeftMouseDown(const glm::vec2& deltaMousePos)
{
	m_ShowMouseCursor = false;

	// Application::Get()->GetWindow()->SetInFocus(true);
	DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor);

	Log::GetLogger()->info("DX11TestLayer::OnLeftMouseDown([{0}, {1}])", deltaMousePos.x, deltaMousePos.y);
	//	bool windowInFocus = Application::Get()->GetWindow()->IsInFocus();
	//	Log::GetLogger()->info("Window::m_InFocus: {0}, m_ShowMouseCursor: {1}, m_Camera->IsEnabled: {2}",
	//		windowInFocus, m_ShowMouseCursor, DX11CameraFP::Get()->IsEnabled());
}

void DX11TestLayer::OnRightMouseDown(const glm::vec2& deltaMousePos)
{
}

void DX11TestLayer::OnLeftMouseUp(const glm::vec2& deltaMousePos)
{
}

void DX11TestLayer::OnRightMouseUp(const glm::vec2& deltaMousePos)
{
}
