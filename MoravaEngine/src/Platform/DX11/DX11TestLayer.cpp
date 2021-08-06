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

ImGuizmo::OPERATION DX11TestLayer::s_ImGuizmoType;

bool DX11TestLayer::s_LeftControlKeyPressed = false;

bool DX11TestLayer::s_ShowWindowSceneHierarchy = true;
bool DX11TestLayer::s_ShowWindowAssetManager = true;
bool DX11TestLayer::s_ShowWindowMaterialEditor = true;

Hazel::Ref<Hazel::HazelScene> DX11TestLayer::s_Scene;

glm::mat4 DX11TestLayer::s_CurrentlySelectedTransform;

float DX11TestLayer::s_ViewportWidth = 0.0f;
float DX11TestLayer::s_ViewportHeight = 0.0f;
glm::vec2 DX11TestLayer::s_ViewportBounds[2];
bool DX11TestLayer::s_AllowViewportCameraEvents = true;

Hazel::SceneHierarchyPanel* DX11TestLayer::s_SceneHierarchyPanel;
Hazel::ContentBrowserPanel* DX11TestLayer::s_ContentBrowserPanel;
MaterialEditorPanel* DX11TestLayer::s_MaterialEditorPanel;


DX11TestLayer::DX11TestLayer()
{
	s_Camera = std::make_shared<DX11CameraFP>(glm::perspectiveFov(glm::radians(60.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
}

DX11TestLayer::DX11TestLayer(const std::string& name) : MoravaLayer(name)
{
	s_Camera = std::make_shared<DX11CameraFP>(glm::perspectiveFov(glm::radians(60.0f), 1280.0f, 720.0f, 0.1f, 1000.0f));
}

DX11TestLayer::~DX11TestLayer()
{
}

void DX11TestLayer::OnAttach()
{
	DX11InputSystem::Get()->AddListener(this);

	s_Scene = Hazel::Ref<Hazel::HazelScene>::Create();

	s_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel(s_Scene);

	s_ContentBrowserPanel = new Hazel::ContentBrowserPanel();

	s_MaterialEditorPanel = new MaterialEditorPanel();

	// Application::Get()->GetWindow()->SetInFocus(false);

	DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor = true);

	Hazel::Ref<Hazel::HazelMesh> meshSphere = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/sphere_hq.obj");

	/*
	RenderObject renderObjectGladiator;
	renderObjectGladiator.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Gladiator/Gladiator.fbx");
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_weapon_BaseColor.jpg"));
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_weapon_Normal.jpg"));
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_BaseColor.jpg"));
	renderObjectGladiator.Textures.push_back(ResourceManager::LoadHazelTexture2D("Models/Gladiator/Gladiator_Normal.jpg"));
	renderObjectGladiator.Transform = glm::mat4(1.0f);
	renderObjectGladiator.Transform = glm::translate(renderObjectGladiator.Transform, glm::vec3(0.0f, 0.0f, -2.0f));
	renderObjectGladiator.Transform = glm::scale(renderObjectGladiator.Transform, glm::vec3(0.04f));
	renderObjectGladiator.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectGladiator);

	RenderObject renderObjectCerberus;
	renderObjectCerberus.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx");
	renderObjectCerberus.Textures.push_back(renderObjectCerberus.Mesh->GetTextures().at(0));
	renderObjectCerberus.Textures.push_back(renderObjectCerberus.Mesh->GetTextures().at(1));
	renderObjectCerberus.Transform = glm::mat4(1.0f);
	renderObjectCerberus.Transform = glm::translate(renderObjectCerberus.Transform, glm::vec3(0.0f, 4.0f, 14.0f));
	renderObjectCerberus.Transform = glm::rotate(renderObjectCerberus.Transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	renderObjectCerberus.Transform = glm::rotate(renderObjectCerberus.Transform, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	renderObjectCerberus.Transform = glm::scale(renderObjectCerberus.Transform, glm::vec3(4.0f));
	renderObjectCerberus.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectCerberus);

	RenderObject renderObjectSphereLeft;
	renderObjectSphereLeft.Mesh = meshSphere;
	renderObjectSphereLeft.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg"));
	renderObjectSphereLeft.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg"));
	renderObjectSphereLeft.Transform = glm::mat4(1.0f);
	renderObjectSphereLeft.Transform = glm::translate(renderObjectSphereLeft.Transform, glm::vec3(-4.0f, 2.0f, 0.0f));
	renderObjectSphereLeft.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectSphereLeft);

	RenderObject renderObjectSphereRight;
	renderObjectSphereRight.Mesh = meshSphere;
	renderObjectSphereRight.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg"));
	renderObjectSphereRight.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg"));
	renderObjectSphereRight.Transform = glm::mat4(1.0f);
	renderObjectSphereRight.Transform = glm::translate(renderObjectSphereRight.Transform, glm::vec3(4.0f, 2.0f, 0.0f));
	renderObjectSphereRight.PipelineType = RenderObject::PipelineType::Light;
	m_RenderObjects.push_back(renderObjectSphereRight);
	*/

	RenderObject renderObjectTerrain;
	renderObjectTerrain.Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/terrain.obj");
	renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/sand.jpg"));
	renderObjectTerrain.Textures.push_back(ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png"));
	renderObjectTerrain.Transform = glm::mat4(1.0f);
	renderObjectTerrain.Transform = glm::scale(renderObjectTerrain.Transform, glm::vec3(4.0f));
	renderObjectTerrain.PipelineType = RenderObject::PipelineType::Unlit;
	m_RenderObjects.push_back(renderObjectTerrain);

	// ---- other assets ----
	ResourceManager::LoadHazelTexture2D("Textures/PardCode/sky.jpg");

	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/wood.jpg");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick.png");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_d.jpg");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/brick_n.jpg");
	// ResourceManager::LoadHazelTexture2D("Textures/default_material_albedo.png");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/normal_blank.png");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/umhlanga_sunrise_4k.jpg");
	// ResourceManager::LoadHazelTexture2D("Textures/PardCode/gold.png");
	// ResourceManager::LoadHazelTexture2D("Textures/container/container2.png");
	// ResourceManager::LoadHazelTexture2D("Textures/container/container2_normal.png");

	s_Mesh = Hazel::Ref<DX11Mesh>::Create(L"Models/PardCode/teapot.obj");
	// s_Mesh = Hazel::Ref<DX11Mesh>::Create(L"Models/PardCode/spaceship.obj");

	s_MeshLight = meshSphere;
	s_SkyboxSphere = meshSphere;

	/**** BEGIN Pipeline Unlit ****/

	Hazel::PipelineSpecification pipelineSpecUnlit;
	pipelineSpecUnlit.DebugName = "Pipeline Unlit";
	pipelineSpecUnlit.Layout = Hazel::VertexBufferLayout{};

	MoravaShaderSpecification moravaShaderSpecificationUnlit;
	moravaShaderSpecificationUnlit.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	moravaShaderSpecificationUnlit.VertexShaderPath = "Shaders/HLSL/UnlitVertexShader.hlsl";
	moravaShaderSpecificationUnlit.PixelShaderPath = "Shaders/HLSL/UnlitPixelShader.hlsl";
	moravaShaderSpecificationUnlit.ForceCompile = false;
	ResourceManager::CreateOrLoadShader(moravaShaderSpecificationUnlit);
	pipelineSpecUnlit.Shader = ResourceManager::CreateOrLoadShader(moravaShaderSpecificationUnlit);

	Hazel::Ref<DX11Pipeline> pipelineUnlit = DX11Pipeline::Create(pipelineSpecUnlit);

	/**** END Pipeline Unlit ****/

	/**** BEGIN Pipeline Illuminated ****/

	Hazel::PipelineSpecification pipelineSpecIlluminated;
	pipelineSpecIlluminated.DebugName = "Pipeline Illuminated";
	pipelineSpecIlluminated.Layout = Hazel::VertexBufferLayout{};

	MoravaShaderSpecification moravaShaderSpecificationIlluminated;
	moravaShaderSpecificationIlluminated.ShaderType = MoravaShaderSpecification::ShaderType::DX11Shader;
	moravaShaderSpecificationIlluminated.VertexShaderPath = "Shaders/HLSL/DirLightVertexShader.hlsl";
	moravaShaderSpecificationIlluminated.PixelShaderPath = "Shaders/HLSL/DirLightPixelShader.hlsl";
	moravaShaderSpecificationIlluminated.ForceCompile = false;
	pipelineSpecIlluminated.Shader = ResourceManager::CreateOrLoadShader(moravaShaderSpecificationIlluminated);

	Hazel::Ref<DX11Pipeline> pipelineIlluminated = DX11Pipeline::Create(pipelineSpecIlluminated);

	/**** END Pipeline Illuminated ****/

	/**** BEGIN Create meshes with materials ****

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
	renderObjectHouse.MeshDX11 = Hazel::Ref<DX11Mesh>::Create(L"Models/PardCode/house.obj");
	renderObjectHouse.Transform = glm::mat4(1.0f);
	renderObjectHouse.Transform = glm::translate(renderObjectHouse.Transform, glm::vec3(0.0f, 0.0f, -20.0f));
	renderObjectHouse.Transform = glm::scale(renderObjectHouse.Transform, glm::vec3(6.0f));
	renderObjectHouse.PipelineType = RenderObject::PipelineType::Light;
	s_RenderObjectsWithMaterials.push_back(renderObjectHouse);
	/**** END Create meshes with materials ****/

	// END prepare data for rendering meshes with materials (render objects and the list of materials)
}

void DX11TestLayer::OnDetach()
{
}

void DX11TestLayer::OnUpdate(Hazel::Timestep ts)
{
	bool windowInFocus = Application::Get()->GetWindow()->IsInFocus();
	bool cameraEnabled = windowInFocus; // && !m_ShowMouseCursor;
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

void DX11TestLayer::OnRender(Window* mainWindow, Scene* scene)
{
	DX11Renderer::Draw(scene->GetCamera());
}

void DX11TestLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
}

void DX11TestLayer::Render(const glm::vec4& clearColor, std::shared_ptr<DX11CameraFP> camera)
{
}

void DX11TestLayer::OnKeyDown(int key)
{
	if (key == VK_LCONTROL)
	{
		s_LeftControlKeyPressed = true;
	}
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

	// ImGizmo switching modes
	switch (key)
	{
		case '1':
			s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case '2':
			s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		case '3':
			s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		case '4':
			s_ImGuizmoType = (ImGuizmo::OPERATION)-1;
			break;
	}

	if (key == VK_LCONTROL)
	{
		s_LeftControlKeyPressed = false;
	}

	if (s_LeftControlKeyPressed)
	{
		if (key == 'H')
		{
			s_ShowWindowSceneHierarchy = !s_ShowWindowSceneHierarchy;
			Log::GetLogger()->info("s_ShowWindowSceneHierarchy: {0}", s_ShowWindowSceneHierarchy);
		}

		if (key == VK_SPACE)
		{
			s_ShowWindowAssetManager = !s_ShowWindowAssetManager;
			Log::GetLogger()->info("s_ShowWindowAssetManager: {0}", s_ShowWindowAssetManager);
		}

		if (key == 'M')
		{
			s_ShowWindowMaterialEditor = !s_ShowWindowMaterialEditor;
			Log::GetLogger()->info("s_ShowWindowMaterialEditor: {0}", s_ShowWindowMaterialEditor);
		}
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

	// DX11InputSystem::Get()->ShowCursor(m_ShowMouseCursor = false);

	//	Log::GetLogger()->info("DX11TestLayer::OnLeftMouseDown {0}x{1}", mousePos.x, mousePos.y);
	//	bool windowInFocus = Application::Get()->GetWindow()->IsInFocus();
	//	Log::GetLogger()->info("Window::m_InFocus: {0}, m_ShowMouseCursor: {1}, m_Camera->IsEnabled: {2}",
	//		windowInFocus, m_ShowMouseCursor, DX11CameraFP::Get()->IsEnabled());

	OnLeftMouseDownEventHandler(mousePos);
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

bool DX11TestLayer::OnLeftMouseDownEventHandler(const glm::vec2& mousePos)
{
	float mx = mousePos.x;
	float my = mousePos.y;

	Log::GetLogger()->debug("DX11TestLayer::OnLeftMouseDownEventHandler mousePos.x: {0}, mousePos.y: {1}", mousePos.x, mousePos.y);

	if (!ImGuizmo::IsUsing() && !ImGuizmo::IsOver())
	{
		auto [mouseX, mouseY] = GetMouseViewportSpace();

		Log::GetLogger()->debug("DX11TestLayer::OnLeftMouseDownEventHandler GetMouseViewportSpace mouseX: {0}, mouseY: {1}", mouseX, mouseY);

		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
		{
			auto [origin, direction] = CastRay(mouseX, mouseY);

			EntitySelection::s_SelectionContext.clear();

			auto meshEntities = s_Scene->GetAllEntitiesWith<Hazel::MeshComponent>();
			for (auto e : meshEntities)
			{
				Hazel::Entity entity = { e, s_Scene.Raw() };
				auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
				if (!mesh) {
					continue;
				}

				std::vector<Hazel::Submesh>& submeshes = mesh->GetSubmeshes();
				float lastT = std::numeric_limits<float>::max(); // Distance between camera and intersection in CastRay
				// for (Hazel::Submesh& submesh : submeshes)
				for (uint32_t i = 0; i < submeshes.size(); i++)
				{
					Hazel::Submesh* submesh = &submeshes[i];
					auto transform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
					Hazel::Ray ray = {
						glm::inverse(transform * submesh->Transform) * glm::vec4(origin, 1.0f),
						glm::inverse(glm::mat3(transform) * glm::mat3(submesh->Transform)) * direction
					};

					float t;
					bool intersects = ray.IntersectsAABB(submesh->BoundingBox, t);
					if (intersects)
					{
						const auto& triangleCache = ((Hazel::HazelMesh*)mesh.Raw())->GetTriangleCache(i);
						if (triangleCache.size())
						{
							for (const auto& triangle : triangleCache)
							{
								if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
								{
									AddSubmeshToSelectionContext({ entity, submesh, t });

									Log::GetLogger()->debug("Adding submesh to selection context. Submesh Name: '{0}', selection size: '{1}'",
										submesh->MeshName, EntitySelection::s_SelectionContext.size());
									break;
								}
							}
						}
						else {
							AddSubmeshToSelectionContext({ entity, submesh, t });
						}
					}
				}
			}
			std::sort(EntitySelection::s_SelectionContext.begin(), EntitySelection::s_SelectionContext.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });

			// TODO: Handle mesh being deleted, etc
			if (EntitySelection::s_SelectionContext.size()) {
				s_CurrentlySelectedTransform = EntitySelection::s_SelectionContext[0].Mesh->Transform;
				OnSelected(EntitySelection::s_SelectionContext[0]);
			}
			else {
				Ref<Hazel::Entity> meshEntity = GetMeshEntity();
				if (meshEntity) {
					s_CurrentlySelectedTransform = meshEntity->Transform().GetTransform();
				}
			}
		}
	}
	return false;
}

std::pair<float, float> DX11TestLayer::GetMouseViewportSpace()
{
	auto [mx, my] = ImGui::GetMousePos(); // Input::GetMousePosition();
	mx -= s_ViewportBounds[0].x;
	my -= s_ViewportBounds[0].y;
	s_ViewportWidth  = s_ViewportBounds[1].x - s_ViewportBounds[0].x;
	s_ViewportHeight = s_ViewportBounds[1].y - s_ViewportBounds[0].y;

	return { (mx / s_ViewportWidth) * 2.0f - 1.0f, ((my / s_ViewportHeight) * 2.0f - 1.0f) * -1.0f };
}

std::pair<glm::vec3, glm::vec3> DX11TestLayer::CastRay(float mx, float my)
{
	glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

	glm::mat4 projectionMatrix = s_Camera->GetProjectionMatrix();
	glm::mat4 viewMatrix = s_Camera->GetViewMatrix();

	auto inverseProj = glm::inverse(projectionMatrix);
	auto inverseView = glm::inverse(glm::mat3(viewMatrix));

	glm::vec4 ray = inverseProj * mouseClipPos;
	glm::vec3 rayPos = s_Camera->GetPosition();
	glm::vec3 rayDir = inverseView * glm::vec3(ray); // inverseView * glm::vec3(ray)

	Log::GetLogger()->debug("DX11TestLayer::CastRay | MousePosition [ {0} {1} ]", mx, my);
	Log::GetLogger()->debug("DX11TestLayer::CastRay | m_ViewportBounds[0] [ {0} {1} ]", s_ViewportBounds[0].x, s_ViewportBounds[0].y);
	Log::GetLogger()->debug("DX11TestLayer::CastRay | m_ViewportBounds[1] [ {0} {1} ]", s_ViewportBounds[1].x, s_ViewportBounds[1].y);
	Log::GetLogger()->debug("DX11TestLayer::CastRay | mouseClipPos [ {0} {1} ]", mouseClipPos.x, mouseClipPos.y);

	return { rayPos, rayDir };
}

void DX11TestLayer::AddSubmeshToSelectionContext(SelectedSubmesh submesh)
{
	EntitySelection::s_SelectionContext.push_back(submesh);

	if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Mesh != nullptr) {
		Log::GetLogger()->debug("SelectionContext[0].Mesh->MeshName: '{0}'", EntitySelection::s_SelectionContext[0].Mesh->MeshName);
	}
}

void DX11TestLayer::OnSelected(const SelectedSubmesh& selectionContext)
{
	// TODO: move to SceneHazelEnvMap
	s_SceneHierarchyPanel->SetSelected(selectionContext.Entity);
	s_Scene->SetSelectedEntity(selectionContext.Entity);
}

Ref<Hazel::Entity> DX11TestLayer::GetMeshEntity()
{
	Ref<Hazel::Entity> meshEntity;
	auto meshEntities = s_Scene->GetAllEntitiesWith<Hazel::MeshComponent>();
	if (meshEntities.size()) {
		for (auto entt : meshEntities)
		{
			meshEntity = CreateRef<Hazel::Entity>(entt, s_Scene.Raw());
		}
		return meshEntity;
	}
	return nullptr;
}
