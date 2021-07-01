#include "DX11TestLayer.h"

#include "DX11Context.h"
#include "DX11SwapChain.h"
#include "DX11Renderer.h"
#include "DX11Shader.h"
#include "DX11InputSystem.h"

#include "Core/Application.h"
#include "Core/ResourceManager.h"

#include "Hazel/Renderer/HazelTexture.h"


std::shared_ptr<DX11CameraFP> DX11TestLayer::s_Camera;
glm::vec2 DX11TestLayer::s_StartMousePosition;
Hazel::Ref<DX11Mesh> DX11TestLayer::s_Mesh;
Hazel::Ref<Hazel::HazelMesh> DX11TestLayer::s_MeshLight;
Hazel::Ref<Hazel::HazelMesh> DX11TestLayer::s_SkyboxSphere;


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
	DX11InputSystem::Get()->AddListener(this);

	// Application::Get()->GetWindow()->SetInFocus(false);

	DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor = true);

	RenderObject renderObjectGladiator;
	renderObjectGladiator.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Gladiator/Gladiator.fbx");
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_weapon_BaseColor.jpg"));
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_weapon_Normal.jpg"));
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_BaseColor.jpg"));
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_Normal.jpg"));
	renderObjectGladiator.Transform = glm::mat4(1.0f);
	renderObjectGladiator.Transform = glm::translate(renderObjectGladiator.Transform, glm::vec3(0.0f, 0.0f, -4.0f));
	renderObjectGladiator.Transform = glm::scale(renderObjectGladiator.Transform, glm::vec3(0.04f));
	m_RenderObjects.push_back(renderObjectGladiator);

	RenderObject renderObjectCerberus;
	renderObjectCerberus.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx");
	renderObjectCerberus.Textures.push_back(renderObjectCerberus.Mesh->GetTextures().at(0));
	renderObjectCerberus.Textures.push_back(renderObjectCerberus.Mesh->GetTextures().at(1));
	renderObjectCerberus.Transform = glm::mat4(1.0f);
	renderObjectCerberus.Transform = glm::translate(renderObjectCerberus.Transform, glm::vec3(0.0f, 4.0f, 4.0f));
	renderObjectCerberus.Transform = glm::rotate(renderObjectCerberus.Transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	renderObjectCerberus.Transform = glm::rotate(renderObjectCerberus.Transform, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	renderObjectCerberus.Transform = glm::scale(renderObjectCerberus.Transform, glm::vec3(2.0f));
	m_RenderObjects.push_back(renderObjectCerberus);

	Hazel::Ref<Hazel::HazelMesh> meshSphere = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/sphere_hq.obj");

	RenderObject renderObjectSphereLeft;
	renderObjectSphereLeft.Mesh = meshSphere;
	renderObjectSphereLeft.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg"));
	renderObjectSphereLeft.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg"));
	renderObjectSphereLeft.Transform = glm::mat4(1.0f);
	renderObjectSphereLeft.Transform = glm::translate(renderObjectSphereLeft.Transform, glm::vec3(-4.0f, 2.0f, -4.0f));
	m_RenderObjects.push_back(renderObjectSphereLeft);

	RenderObject renderObjectSphereRight;
	renderObjectSphereRight.Mesh = meshSphere;
	renderObjectSphereRight.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg"));
	renderObjectSphereRight.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg"));
	renderObjectSphereRight.Transform = glm::mat4(1.0f);
	renderObjectSphereRight.Transform = glm::translate(renderObjectSphereRight.Transform, glm::vec3(4.0f, 2.0f, -4.0f));
	m_RenderObjects.push_back(renderObjectSphereRight);

	RenderObject renderObjectTerrain;
	renderObjectTerrain.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/terrain.obj");
	// renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PBR/grass/albedo.png"));
	// renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PBR/grass/normal.png"));
	renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/grass.jpg"));
	renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png"));
	renderObjectTerrain.Transform = glm::mat4(1.0f);
	renderObjectTerrain.Transform = glm::scale(renderObjectTerrain.Transform, glm::vec3(0.5f));
	m_RenderObjects.push_back(renderObjectTerrain);

	// ---- other assets ----
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/wood.jpg");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick.png");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg");
	ResourceManager::LoadHazelTexture2D("Textures/default_material_albedo.png");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/sky.jpg");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/umhlanga_sunrise_4k.jpg");
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/gold.png");
	ResourceManager::LoadHazelTexture2D("Textures/container/container2.png");
	ResourceManager::LoadHazelTexture2D("Textures/container/container2_normal.png");

	s_Mesh = Hazel::Ref<DX11Mesh>::Create(L"Models/PardCode/teapot.obj");
	s_MeshLight = meshSphere;
	s_SkyboxSphere = meshSphere;
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
		glm::perspectiveFov(glm::radians(60.0f), (float)DX11Renderer::GetViewportWidth(), (float)DX11Renderer::GetViewportHeight(), 0.01f, 1000.0f));

	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

	Render(clearColor, s_Camera);

	for (RenderObject renderObject : m_RenderObjects)
	{
		DX11Renderer::SubmitMesh(renderObject);
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
			s_Camera->SetProjectionMatrix(glm::perspectiveFov(glm::radians(60.0f), (float)e.GetWidth(), (float)e.GetHeight(), 0.1f, 1000.0f));
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
}

void DX11TestLayer::OnKeyDown(int key)
{
}

void DX11TestLayer::OnKeyUp(int key)
{
	if (key == VK_ESCAPE)
	{
		// Application::Get()->GetWindow()->SetInFocus(false);
		DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor = true);
	}
}

void DX11TestLayer::OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs)
{
}

void DX11TestLayer::OnLeftMouseDown(const glm::vec2& mousePos)
{
	// MOUSE events
	POINT currentMousePos = {};
	::GetCursorPos(&currentMousePos);
	s_StartMousePosition = glm::vec2(currentMousePos.x, currentMousePos.y);

	if (DX11InputSystem::Get()->IsMouseCursorAboveViewport())
	{
		Application::Get()->GetWindow()->SetInFocus(true);
	}

	DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor = false);

	//	Log::GetLogger()->info("DX11TestLayer::OnLeftMouseDown {0}x{1}", mousePos.x, mousePos.y);
	//	bool windowInFocus = Application::Get()->GetWindow()->IsInFocus();
	//	Log::GetLogger()->info("Window::m_InFocus: {0}, m_ShowMouseCursor: {1}, m_Camera->IsEnabled: {2}",
	//		windowInFocus, m_ShowMouseCursor, DX11CameraFP::Get()->IsEnabled());
}

void DX11TestLayer::OnRightMouseDown(const glm::vec2& mousePos)
{
}

void DX11TestLayer::OnLeftMouseUp(const glm::vec2& mousePos)
{
}

void DX11TestLayer::OnRightMouseUp(const glm::vec2& mousePos)
{
}
