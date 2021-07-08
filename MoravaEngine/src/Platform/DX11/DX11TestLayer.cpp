#include "DX11TestLayer.h"

#include "DX11Context.h"
#include "DX11SwapChain.h"
#include "DX11Renderer.h"
#include "DX11Shader.h"
#include "DX11InputSystem.h"

#include "Core/Application.h"
#include "Core/ResourceManager.h"
#include "Platform/Windows/WindowsWindow.h"

#include "Hazel/Renderer/HazelTexture.h"


std::shared_ptr<DX11CameraFP> DX11TestLayer::s_Camera;
glm::vec2 DX11TestLayer::s_StartMousePosition;
Hazel::Ref<DX11Mesh> DX11TestLayer::s_Mesh;
Hazel::Ref<Hazel::HazelMesh> DX11TestLayer::s_MeshLight;
Hazel::Ref<Hazel::HazelMesh> DX11TestLayer::s_SkyboxSphere;
// Render meshes with materials
std::vector<RenderObject> DX11TestLayer::s_RenderObjectsWithMaterials;
std::vector<Hazel::Ref<DX11Material>> DX11TestLayer::s_ListMaterials;


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
	renderObjectGladiator.PipelineType = RenderObject::PipelineType::Light;
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
	renderObjectCerberus.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectCerberus);

	Hazel::Ref<Hazel::HazelMesh> meshSphere = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/sphere_hq.obj");

	RenderObject renderObjectSphereLeft;
	renderObjectSphereLeft.Mesh = meshSphere;
	renderObjectSphereLeft.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg"));
	renderObjectSphereLeft.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg"));
	renderObjectSphereLeft.Transform = glm::mat4(1.0f);
	renderObjectSphereLeft.Transform = glm::translate(renderObjectSphereLeft.Transform, glm::vec3(-4.0f, 2.0f, -4.0f));
	renderObjectSphereLeft.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectSphereLeft);

	RenderObject renderObjectSphereRight;
	renderObjectSphereRight.Mesh = meshSphere;
	renderObjectSphereRight.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg"));
	renderObjectSphereRight.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg"));
	renderObjectSphereRight.Transform = glm::mat4(1.0f);
	renderObjectSphereRight.Transform = glm::translate(renderObjectSphereRight.Transform, glm::vec3(4.0f, 2.0f, -4.0f));
	renderObjectSphereRight.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectSphereRight);

	RenderObject renderObjectTerrain;
	renderObjectTerrain.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/terrain.obj");
	// renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PBR/grass/albedo.png"));
	// renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PBR/grass/normal.png"));
	renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/sand.jpg"));
	renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png"));
	renderObjectTerrain.Transform = glm::mat4(1.0f);
	renderObjectTerrain.Transform = glm::scale(renderObjectTerrain.Transform, glm::vec3(0.5f));
	renderObjectTerrain.PipelineType = RenderObject::PipelineType::Unlit;
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
	// s_Mesh = Hazel::Ref<DX11Mesh>::Create(L"Models/PardCode/spaceship.obj");

	s_MeshLight = meshSphere;
	s_SkyboxSphere = meshSphere;

	// BEGIN Pipeline Unlit
	Hazel::PipelineSpecification pipelineSpecUnlit;
	pipelineSpecUnlit.DebugName = "Pipeline Unlit";
	pipelineSpecUnlit.Layout = Hazel::VertexBufferLayout{};

	MoravaShaderSpecification shaderSpecUnlit;
	shaderSpecUnlit.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	shaderSpecUnlit.VertexShaderPath = "Shaders/HLSL/UnlitVertexShader.hlsl";
	shaderSpecUnlit.PixelShaderPath = "Shaders/HLSL/UnlitPixelShader.hlsl";
	shaderSpecUnlit.ForceCompile = false;
	pipelineSpecUnlit.Shader = DX11Shader::Create(shaderSpecUnlit);

	Hazel::Ref<DX11Pipeline> pipelineUnlit = DX11Pipeline::Create(pipelineSpecUnlit);
	// END Pipeline Unlit

	// BEGIN Pipeline Illuminated
	Hazel::PipelineSpecification pipelineSpecIlluminated;
	pipelineSpecIlluminated.DebugName = "Pipeline Illuminated";
	pipelineSpecIlluminated.Layout = Hazel::VertexBufferLayout{};

	MoravaShaderSpecification shaderSpecIlluminated;
	shaderSpecIlluminated.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	shaderSpecIlluminated.VertexShaderPath = "Shaders/HLSL/DirLightVertexShader.hlsl";
	shaderSpecIlluminated.PixelShaderPath = "Shaders/HLSL/DirLightPixelShader.hlsl";
	shaderSpecIlluminated.ForceCompile = false;
	pipelineSpecIlluminated.Shader = DX11Shader::Create(shaderSpecIlluminated);

	Hazel::Ref<DX11Pipeline> pipelineIlluminated = DX11Pipeline::Create(pipelineSpecIlluminated);
	// END Pipeline Illuminated

	Hazel::Ref<DX11Material> materialIlluminated = Hazel::Ref<DX11Material>::Create(pipelineIlluminated, "Material Illuminated");
	Hazel::Ref<DX11Material> materialUnlit = Hazel::Ref<DX11Material>::Create(pipelineIlluminated, "Material Unlit");

	Hazel::Ref<DX11Material> materialIlluminatedDerived = Hazel::Ref<DX11Material>::Create(materialIlluminated, "Material Illuminated Derived");
	Hazel::Ref<DX11Material> materialUnlitDerived = Hazel::Ref<DX11Material>::Create(materialUnlit, "Material Unlit Derived");

	// BEGIN prepare data for rendering meshes with materials (render objects and the list of materials)
	// std::vector<RenderObject> DX11TestLayer::s_RenderObjectsWithMaterials;
	// std::vector<Hazel::Ref<DX11Material>> DX11TestLayer::s_ListMaterials;

	s_ListMaterials.reserve(32); // reserve 32 slots

	Hazel::Ref<Hazel::HazelTexture2D> textureBarrel       = ResourceManager::LoadHazelTexture2D("Textures/PardCode/barrel.jpg");
	Hazel::Ref<Hazel::HazelTexture2D> textureHouseBrick   = ResourceManager::LoadHazelTexture2D("Textures/PardCode/house_brick.jpg");
	Hazel::Ref<Hazel::HazelTexture2D> textureHouseWindows = ResourceManager::LoadHazelTexture2D("Textures/PardCode/house_windows.jpg");
	Hazel::Ref<Hazel::HazelTexture2D> textureHouseWood    = ResourceManager::LoadHazelTexture2D("Textures/PardCode/house_wood.jpg");

	Hazel::Ref<DX11Material> materialBarrel       = DX11Material::Create(pipelineSpecIlluminated.Shader, "Material Barrel");
	Hazel::Ref<DX11Material> materialHouseBrick   = DX11Material::Create(pipelineSpecIlluminated.Shader, "Material House Brick");
	Hazel::Ref<DX11Material> materialHouseWindows = DX11Material::Create(pipelineSpecIlluminated.Shader, "Material House Windows");
	Hazel::Ref<DX11Material> materialHouseWood    = DX11Material::Create(pipelineSpecIlluminated.Shader, "Material House Wood");

	materialBarrel->AddTexture(textureBarrel.As<DX11Texture2D>());
	materialHouseBrick->AddTexture(textureHouseBrick.As<DX11Texture2D>());
	materialHouseWindows->AddTexture(textureHouseWindows.As<DX11Texture2D>());
	materialHouseWood->AddTexture(textureHouseWood.As<DX11Texture2D>());

	s_ListMaterials.push_back(materialBarrel);
	s_ListMaterials.push_back(materialHouseBrick);
	s_ListMaterials.push_back(materialHouseWindows);
	s_ListMaterials.push_back(materialHouseWood);

	RenderObject renderObjectHouse;
	renderObjectHouse.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/house.obj");
	renderObjectHouse.Transform = glm::mat4(1.0f);
	renderObjectHouse.Transform = glm::translate(renderObjectHouse.Transform, glm::vec3(0.0f, 0.0f, -5.0f));
	renderObjectHouse.PipelineType = RenderObject::PipelineType::Light;
	s_RenderObjectsWithMaterials.push_back(renderObjectHouse);

	// END prepare data for rendering meshes with materials (render objects and the list of materials)
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

	if (key == 'F')
	{
		m_FullscreenEnabled = !m_FullscreenEnabled;

		WindowsWindow* windowsWindow = (WindowsWindow*)Application::Get()->GetWindow();
		RECT windowRECT = windowsWindow->GetSizeScreen();

		uint32_t width = windowRECT.right; // - windowRECT.left;
		uint32_t height = windowRECT.bottom; // - windowRECT.top;

		DX11Context::Get()->GetSwapChain()->SetFullScreen(m_FullscreenEnabled, width, height);
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
