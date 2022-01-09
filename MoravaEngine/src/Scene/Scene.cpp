#define _CRT_SECURE_NO_WARNINGS

#include "Scene/Scene.h"

#include "H2M/Core/Events/ApplicationEventH2M.h"
#include "H2M/Scene/ComponentsH2M.h"

#include "Core/Application.h"
#include "Mesh/MeshData.h"
#include "Mesh/Tile2D.h"
#include "Texture/TextureLoader.h"
#include "Core/Timer.h"
#include "Core/Input.h"
#include "Core/MousePicker.h"


SceneSettings Scene::sceneSettings;

// ImGuizmo
glm::mat4* Scene::s_ImGuizmoTransform = nullptr;
int Scene::s_ImGuizmoType = -1; // -1 = no gizmo


Scene::Scene()
{
	sceneSettings.enableShadows            = false;
	sceneSettings.enableOmniShadows        = false;
	sceneSettings.enableCascadedShadowMaps = false;
	sceneSettings.enablePointLights        = false;
	sceneSettings.enableSpotLights         = false;
	sceneSettings.enableWaterEffects       = false;
	sceneSettings.enableSkybox             = false;
	sceneSettings.enableNormalMaps         = false;
	sceneSettings.enableCulling            = false;
	sceneSettings.enableParticles          = false;
	sceneSettings.cameraPosition           = glm::vec3(0.0f, 6.0f, 20.0f);
	sceneSettings.cameraStartYaw           = -90.0f;
	sceneSettings.cameraStartPitch         = 0.0f;
	sceneSettings.cameraMoveSpeed          = 4.0f;
	sceneSettings.nearPlane                = 0.01f;
	sceneSettings.farPlane                 = 1000.0f;

	// Directional light
	sceneSettings.directionalLight.base.enabled = true;
	sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.directionalLight.base.ambientIntensity = 0.5f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-32.0f, 32.0f, -32.0f, 32.0f, -32.0f, 32.0f);

	// Point lights
	sceneSettings.pointLights[0].base.enabled = true;
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[0].base.diffuseIntensity = 1.0f;
	sceneSettings.pointLights[0].constant = 0.4f;
	sceneSettings.pointLights[0].linear   = 0.3f;
	sceneSettings.pointLights[0].exponent = 0.2f;

	sceneSettings.pointLights[1].base.enabled = true;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pointLights[1].position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[1].base.diffuseIntensity = 1.0f;
	sceneSettings.pointLights[1].constant = 0.4f;
	sceneSettings.pointLights[1].linear   = 0.3f;
	sceneSettings.pointLights[1].exponent = 0.2f;

	sceneSettings.pointLights[2].base.enabled = true;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[2].constant = 0.4f;
	sceneSettings.pointLights[2].linear   = 0.3f;
	sceneSettings.pointLights[2].exponent = 0.2f;

	sceneSettings.pointLights[3].base.enabled = true;
	sceneSettings.pointLights[3].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[3].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[3].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[3].base.diffuseIntensity = 1.0f;
	sceneSettings.pointLights[3].constant = 0.4f;
	sceneSettings.pointLights[3].linear   = 0.3f;
	sceneSettings.pointLights[3].exponent = 0.2f;

	// Spot lights
	sceneSettings.spotLights[0].base.base.enabled = true;
	sceneSettings.spotLights[0].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[0].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[0].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[0].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[0].base.constant = 0.4f;
	sceneSettings.spotLights[0].base.linear   = 0.3f;
	sceneSettings.spotLights[0].base.exponent = 0.2f;
	sceneSettings.spotLights[0].edge = 35.0f;

	sceneSettings.spotLights[1].base.base.enabled = true;
	sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[1].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[1].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[1].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[1].base.constant = 0.4f;
	sceneSettings.spotLights[1].base.linear   = 0.3f;
	sceneSettings.spotLights[1].base.exponent = 0.2f;
	sceneSettings.spotLights[1].edge = 35.0f;

	sceneSettings.spotLights[2].base.base.enabled = true;
	sceneSettings.spotLights[2].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[2].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[2].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[2].base.constant = 0.4f;
	sceneSettings.spotLights[2].base.linear   = 0.3f;
	sceneSettings.spotLights[2].base.exponent = 0.2f;
	sceneSettings.spotLights[2].edge = 35.0f;

	sceneSettings.spotLights[3].base.base.enabled = true;
	sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[3].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[3].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[3].base.constant = 0.4f;
	sceneSettings.spotLights[3].base.linear   = 0.3f;
	sceneSettings.spotLights[3].base.exponent = 0.2f;
	sceneSettings.spotLights[3].edge = 35.0f;

	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.omniShadowMapWidth = 1024;
	sceneSettings.omniShadowMapHeight = 1024;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.waterHeight = 1.6f;
	sceneSettings.waterWaveSpeed = 0.005f;

	m_FOV = 60.0f;
	m_AspectRatio = 16 / 9.0f;

	shadowMapWidth = 1024;
	shadowMapHeight = 1024;

	m_WireframeEnabled = false;

	// Key cooldown time (emulate onKeyReleased)
	m_KeyPressCooldown = { 0.0f, 0.2f };

	// SetLightManager();
	SetupFramebuffers();
	SetupTextures();
	SetupTextureSlots();
	SetupMaterials();
	SetupMeshes();
}

void Scene::Update(float timestep, Window* mainWindow)
{
	Timer::Get()->Update();

	m_Camera->OnUpdate(timestep);
	m_CameraController->Update();

	MousePicker::Get()->Update(m_Camera->GetViewMatrix());

	if (ImGuiWrapper::CanViewportReceiveEvents())
	{
		// Toggle wireframe mode
		if (Input::IsKeyPressed(KeyH2M::R))
		{
			if (Timer::Get()->GetCurrentTimestamp() - m_KeyPressCooldown.lastTime > m_KeyPressCooldown.cooldown)
			{
				m_WireframeEnabled = !m_WireframeEnabled;
				m_KeyPressCooldown.lastTime = Timer::Get()->GetCurrentTimestamp();
			}
		}

		// Flashlight toggle key
		if (Input::IsKeyPressed(KeyH2M::F))
		{
			LightManager::spotLights[2].GetBasePL()->Toggle();
			// Application::Get()->GetWindow()->getKeys()[GLFW_KEY_L] = false;
		}

		// Take a screenshot
		if (Input::IsKeyPressed(KeyH2M::LeftControl) && Input::IsKeyPressed(KeyH2M::P))
		{
			if (Timer::Get()->GetCurrentTimestamp() - m_KeyPressCooldown.lastTime > m_KeyPressCooldown.cooldown)
			{
				time_t rawtime;
				struct tm* timeinfo;
				char buffer[80];

				time(&rawtime);
				timeinfo = localtime(&rawtime);

				strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", timeinfo);
				std::string dateTimeString(buffer);

				Application::Get()->CaptureScreenshot("Screenshots/" + dateTimeString + ".jpg");
				m_KeyPressCooldown.lastTime = Timer::Get()->GetCurrentTimestamp();
			}
		}
	}

	if (m_WireframeEnabled) {
		RendererBasic::EnableWireframe();
	}
	else {
		RendererBasic::DisableWireframe();
	}
}

void Scene::OnWindowResize(H2M::WindowResizeEventH2M& e)
{
	m_CameraController->OnResize((uint32_t)e.GetWidth(), (uint32_t)e.GetHeight());
	m_Camera->SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());
}

void Scene::SetupTextureSlots()
{
	textureSlots.insert(std::make_pair("diffuse",    1));
	textureSlots.insert(std::make_pair("normal",     2));
	textureSlots.insert(std::make_pair("shadow",     3));
	textureSlots.insert(std::make_pair("omniShadow", 4));
	textureSlots.insert(std::make_pair("reflection", 5));
	textureSlots.insert(std::make_pair("refraction", 6));
	textureSlots.insert(std::make_pair("depth",      7));
	textureSlots.insert(std::make_pair("DuDv",       8));

	// PBR
	textureSlots.insert(std::make_pair("albedo",     1));
	textureSlots.insert(std::make_pair("normal",     2));
	textureSlots.insert(std::make_pair("metallic",   3));
	textureSlots.insert(std::make_pair("roughness",  4));
	textureSlots.insert(std::make_pair("ao",         5));

	// environment cubemap
	textureSlots.insert(std::make_pair("equirectangularMap", 6));
	textureSlots.insert(std::make_pair("environmentMap",     7));
}

void Scene::SetupMaterials()
{
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
	{
		materials.insert(std::make_pair("shiny",      new Material(1.0f,  128.0f)));
		materials.insert(std::make_pair("dull",       new Material(1.0f,   64.0f)));
		materials.insert(std::make_pair("superShiny", new Material(1.0f, 1024.0f)));
		materials.insert(std::make_pair("superDull",  new Material(1.0f,   16.0f)));
	}
}

void Scene::SetupMeshes()
{
}

void Scene::SetupParticles()
{
}

void Scene::SetupFramebuffers()
{
}

void Scene::SetupShaders()
{
}

void Scene::SetupModels()
{
}

void Scene::SetSkybox()
{
}

void Scene::SetupTextures()
{
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
	{
		textures.insert(std::make_pair("normalMapDefault", TextureLoader::Get()->GetTexture("Textures/normal_map_default.png", false, false)));
		textures.insert(std::make_pair("shadowMapDefault", TextureLoader::Get()->GetTexture("Textures/shadow_map_default.png", false, false)));
		textures.insert(std::make_pair("waterDuDv", TextureLoader::Get()->GetTexture("Textures/water/waterDuDv.png", false, false)));
		textures.insert(std::make_pair("waterNormal", TextureLoader::Get()->GetTexture("Textures/water/waterNormal.png", false, false)));
	}
}

void Scene::SetCamera()
{
	m_Camera = new Camera(sceneSettings.cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f),
		sceneSettings.cameraStartYaw, sceneSettings.cameraStartPitch);
	
	m_CameraController = new CameraController(m_Camera, m_AspectRatio, sceneSettings.cameraMoveSpeed, 0.1f);
}

void Scene::SetLightManager()
{
	// Skip if Light Manager already initialized
	if (LightManager::pointLightCount > 0 || LightManager::spotLightCount > 0) return;
	LightManager::Init(sceneSettings);
}

void Scene::SetWaterManager(int width, int height)
{
	// Water framebuffers
	m_WaterManager = new WaterManager(width, height, sceneSettings.waterHeight, sceneSettings.waterWaveSpeed);
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void Scene::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags =
		ImGuiDockNodeFlags_None |
		ImGuiDockNodeFlags_PassthruCentralNode |
		ImGuiDockNodeFlags_NoDockingInCentralNode;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", p_open, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	else
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
		ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::SmallButton("click here"))
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	}

	RenderImGuiMenu(mainWindow, dockspace_flags);

	ImGui::End();
}

void Scene::RenderImGuiMenu(Window* mainWindow, ImGuiDockNodeFlags dockspaceFlags)
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit")) mainWindow->SetShouldClose(true);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Undo");
			ImGui::MenuItem("Redo");
			ImGui::MenuItem("Cut");
			ImGui::MenuItem("Copy");
			ImGui::MenuItem("Paste");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Docking"))
		{
			// Disabling fullscreen would allow the window to be moved to the front of other windows, 
			// which we can't undo at the moment without finer window depth/z control.
			//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

			if (ImGui::MenuItem("Flag: NoSplit", "", (dockspaceFlags & ImGuiDockNodeFlags_NoSplit) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoSplit;
			if (ImGui::MenuItem("Flag: NoResize", "", (dockspaceFlags & ImGuiDockNodeFlags_NoResize) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoResize;
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspaceFlags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_PassthruCentralNode;
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspaceFlags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_AutoHideTabBar;
			ImGui::EndMenu();
		}

		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
				" > if io.ConfigDockingWithShift==false (default):" "\n"
				"   drag windows from title bar to dock" "\n"
				" > if io.ConfigDockingWithShift==true:" "\n"
				"   drag windows from anywhere and hold Shift to dock" "\n\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		ImGui::EndMenuBar();
	}
}

Camera* Scene::GetCamera()
{
	return m_Camera;
}

Scene::~Scene()
{
	delete m_CameraController;
	delete m_Camera;
	delete m_WaterManager;

	//	for (auto& texture : textures) // TextureLoader is now responsible for deallocating
	//		if (texture.second != nullptr)
	//			delete texture.second;

	TextureLoader::Get()->Clean();

	//	Objects already deallocated in the Scene destructor
	//	for (auto& mesh : meshes)
	//	{
	//		delete mesh.second;
	//	}

	for (auto& material : materials)
	{
		delete material.second;
	}

	for (auto& model : models)
	{
		delete model.second;
	}

	skyboxFaces.clear();
	textures.clear();
	textureSlots.clear();
	materials.clear();
	meshes.clear();
	models.clear();
}
