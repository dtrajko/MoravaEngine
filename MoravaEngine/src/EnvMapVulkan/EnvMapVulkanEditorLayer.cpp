/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EnvMapVulkanEditorLayer.h"

#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"
#include "H2M/Editor/PanelManagerH2M.h"
#include "H2M/Editor/SceneHierarchyPanelH2M.h"
#include "H2M/Project/ProjectH2M.h"
#include "H2M/Project/ProjectSerializerH2M.h"
#include "H2M/Script/ScriptEngineH2M.h"


#define MAX_PROJECT_NAME_LENGTH 255
#define MAX_PROJECT_FILEPATH_LENGTH 512

static char* s_ProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
static char* s_ProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];

#define SCENE_HIERARCHY_PANEL_ID "SceneHierarchyPanel"
// #define ECS_DEBUG_PANEL_ID "ECSDebugPanel"
// #define CONSOLE_PANEL_ID "EditorConsolePanel"
#define CONTENT_BROWSER_PANEL_ID "ContentBrowserPanel"
// #define PROJECT_SETTINGS_PANEL_ID "ProjectSettingsPanel"


EnvMapVulkanEditorLayer::EnvMapVulkanEditorLayer(const H2M::RefH2M<H2M::UserPreferencesH2M>& userPreferences)
	: m_UserPreferences(userPreferences)
{
}

EnvMapVulkanEditorLayer::~EnvMapVulkanEditorLayer()
{
}

void EnvMapVulkanEditorLayer::OnAttach()
{
	Log::GetLogger()->debug("EnvMapVulkanEditorLayer::OnAttach");

	// Editor
	H2M::TexturePropertiesH2M clampProps;
	clampProps.SamplerWrap = H2M::TextureWrapH2M::Clamp;

	m_CheckerboardTex = H2M::Texture2D_H2M::Create("Resources/Editor/Checkerboard.tga", false);
	m_PlayButtonTex = H2M::Texture2D_H2M::Create("Resources/Editor/PlayButton.png", false);
	m_PauseButtonTex = H2M::Texture2D_H2M::Create("Resources/Editor/PauseButton.png", false);
	m_StopButtonTex = H2M::Texture2D_H2M::Create("Resources/Editor/StopButton.png", false);
	m_SimulateButtonTex = H2M::Texture2D_H2M::Create("Resources/Editor/SimulateButton.png", false);
	m_SelectToolTex = H2M::Texture2D_H2M::Create("Resources/Editor/icon_pointer.png", clampProps);
	m_MoveToolTex = H2M::Texture2D_H2M::Create("Resources/Editor/MoveTool.png", false);
	m_RotateToolTex = H2M::Texture2D_H2M::Create("Resources/Editor/icon_rotate.png", false);
	m_ScaleToolTex = H2M::Texture2D_H2M::Create("Resources/Editor/ScaleTool.png", clampProps);

	m_LogoTex = H2M::Texture2D_H2M::Create("Resources/Editor/H_logo.png", clampProps);
	m_IconMinimize = H2M::Texture2D_H2M::Create("Resources/Editor/window_minimize.png", clampProps);
	m_IconMaximize = H2M::Texture2D_H2M::Create("Resources/Editor/window_maximize.png", clampProps);
	m_IconRestore = H2M::Texture2D_H2M::Create("Resources/Editor/window_restore.png", clampProps);
	m_IconClose = H2M::Texture2D_H2M::Create("Resources/Editor/window_close.png", clampProps);

	m_PointLightIcon = H2M::Texture2D_H2M::Create("Resources/Editor/Icons/PointLight.png", false);

	/////////// Configure Panels ///////////
	m_PanelManager = CreateScopeH2M<H2M::PanelManagerH2M>();

	H2M::RefH2M<H2M::SceneHierarchyPanelH2M> sceneHierarchyPanel = H2M::RefH2M<H2M::SceneHierarchyPanelH2M>::Create(m_EditorScene);
	sceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EnvMapVulkanEditorLayer::SelectEntity, this, std::placeholders::_1));
	sceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EnvMapVulkanEditorLayer::OnEntityDeleted, this, std::placeholders::_1));
	sceneHierarchyPanel->SetMeshAssetConvertCallback(std::bind(&EnvMapVulkanEditorLayer::OnCreateMeshFromMeshSource, this, std::placeholders::_1, std::placeholders::_2));
	sceneHierarchyPanel->SetInvalidMetadataCallback(std::bind(&EnvMapVulkanEditorLayer::SceneHierarchyInvalidMetadataCallback, this, std::placeholders::_1, std::placeholders::_2));

	// m_PanelManager->AddPanel<ECSDebugPanel>(ECS_DEBUG_PANEL_ID, "ECS Debug", false, m_EditorScene);
	// m_PanelManager->AddPanel<EditorConsolePanel>(CONSOLE_PANEL_ID, "Log", true);
	m_PanelManager->AddPanel<H2M::ContentBrowserPanelH2M>(CONTENT_BROWSER_PANEL_ID, "Content Browser", true);
	// m_PanelManager->AddPanel<ProjectSettingsWindow>(PROJECT_SETTINGS_PANEL_ID, "Project Settings", false);

	H2M::RefH2M<H2M::ContentBrowserPanelH2M> contentBrowserPanel = H2M::RefH2M<H2M::ContentBrowserPanelH2M>::Create();

	////////////////////////////////////////////////////////

	m_Renderer2D = H2M::RefH2M<H2M::Renderer2D_H2M>::Create();

	if (!m_UserPreferences->StartupProject.empty())
	{
		OpenProject(m_UserPreferences->StartupProject);
	}
	else
	{
		H2M_CORE_VERIFY(false, "No project provided!");
	}

	m_ViewportRenderer = H2M::RefH2M<EnvMapVulkanSceneRenderer>::Create(m_CurrentScene);
	m_SecondViewportRenderer = H2M::RefH2M<EnvMapVulkanSceneRenderer>::Create(m_CurrentScene);
	m_FocusedRenderer = m_ViewportRenderer;

	// AssetEditorPanel::RegisterDefaultEditors();

	m_Renderer2D->SetLineWidth(m_LineWidth);
	m_ViewportRenderer->SetLineWidth(m_LineWidth);
	UpdateSceneRendererSettings();
	// AudioEventsEditor::Init();

	// UI::Widgets::Init();
	H2M::SceneHierarchyPanelH2M::Init();

	// s_HazelInstallPath = FileSystem::GetEnvironmentVariable("HAZEL_DIR");
}

void EnvMapVulkanEditorLayer::OnDetach()
{
}

void EnvMapVulkanEditorLayer::OnUpdate(H2M::TimestepH2M ts)
{
}

void EnvMapVulkanEditorLayer::OnRender()
{
	Log::GetLogger()->error("Method not implemented. TODO: Check if the method LayerH2M::OnRender is deprecated and why...");
}

void EnvMapVulkanEditorLayer::OnImGuiRender()
{
}

void EnvMapVulkanEditorLayer::OnEvent(H2M::EventH2M& e)
{
}

void EnvMapVulkanEditorLayer::OnRender2D()
{
}

bool EnvMapVulkanEditorLayer::OnKeyPressedEvent(H2M::KeyPressedEventH2M& e)
{
	return false;
}

bool EnvMapVulkanEditorLayer::OnMouseButtonPressed(H2M::MouseButtonPressedEventH2M& e)
{
	return false;
}

void EnvMapVulkanEditorLayer::SelectEntity(H2M::EntityH2M entity)
{
}

void EnvMapVulkanEditorLayer::OpenProject()
{
}

void EnvMapVulkanEditorLayer::OpenProject(const std::string& filepath)
{
	if (H2M::ProjectH2M::GetActive())
	{
		CloseProject();
	}

	H2M::RefH2M<H2M::ProjectH2M> project = H2M::RefH2M<H2M::ProjectH2M>::Create();
	H2M::ProjectSerializerH2M serializer(project);
	serializer.Deserialize(filepath);
	H2M::ProjectH2M::SetActive(project);

	// H2M::ScriptEngineH2M::LoadAppAssembly((H2M::ProjectH2M::GetScriptModuleFilePath()).string());

	m_PanelManager->OnProjectChanged(project);

	if (!project->GetConfig().StartScene.empty())
	{
		OpenScene((H2M::ProjectH2M::GetAssetDirectory() / project->GetConfig().StartScene).string());
	}
	else
	{
		NewScene();
	}

	if (m_EditorScene)
	{
		m_EditorScene->SetSelectedEntity({});
	}
	m_SelectionContext.clear();

	// H2M::FileSystemH2M::StartWatching();

	// Reset cameras
	m_EditorCamera = H2M::EditorCameraH2M(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
	m_SecondEditorCamera = H2M::EditorCameraH2M(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);

	memset(s_ProjectNameBuffer, 0, MAX_PROJECT_NAME_LENGTH);
	memset(s_ProjectFilePathBuffer, 0, MAX_PROJECT_FILEPATH_LENGTH);
}

void EnvMapVulkanEditorLayer::CreateProject(std::filesystem::path projectPath)
{
}

void EnvMapVulkanEditorLayer::SaveProject()
{
}

void EnvMapVulkanEditorLayer::CloseProject(bool unloadProject)
{
}

void EnvMapVulkanEditorLayer::NewScene(const std::string& name)
{
}

void EnvMapVulkanEditorLayer::OpenScene()
{
}

void EnvMapVulkanEditorLayer::OpenScene(const std::string& filepath)
{
}

void EnvMapVulkanEditorLayer::OpenScene(const H2M::AssetMetadataH2M& assetMetadata)
{
}

void EnvMapVulkanEditorLayer::SaveScene()
{
}

void EnvMapVulkanEditorLayer::SaveSceneAs()
{
}

void EnvMapVulkanEditorLayer::UI_WelcomePopup()
{
}

void EnvMapVulkanEditorLayer::UI_AboutPopup()
{
}

void EnvMapVulkanEditorLayer::UI_CreateNewMeshPopup()
{
}

void EnvMapVulkanEditorLayer::UI_InvalidAssetMetadataPopup()
{
}

void EnvMapVulkanEditorLayer::UI_NewScene()
{
}

void EnvMapVulkanEditorLayer::SceneHierarchyInvalidMetadataCallback(H2M::EntityH2M entity, H2M::AssetHandleH2M handle)
{
}

std::pair<float, float> EnvMapVulkanEditorLayer::GetMouseViewportSpace(bool primaryViewport)
{
	return std::pair<float, float>();
}

std::pair<glm::vec3, glm::vec3> EnvMapVulkanEditorLayer::CastRay(const H2M::EditorCameraH2M& camera, float mx, float my)
{
	return std::pair<glm::vec3, glm::vec3>();
}

void EnvMapVulkanEditorLayer::OnSelected(const SelectedSubmesh& selectionContext)
{
}

void EnvMapVulkanEditorLayer::OnEntityDeleted(H2M::EntityH2M e)
{
}

void EnvMapVulkanEditorLayer::OnCreateMeshFromMeshSource(H2M::EntityH2M entity, H2M::RefH2M<H2M::MeshH2M> mesh)
{
	m_ShowCreateNewMeshPopup = true;
	m_CreateNewMeshPopupData.MeshToCreate = mesh;
	m_CreateNewMeshPopupData.TargetEntity = entity;
}

H2M::RayH2M EnvMapVulkanEditorLayer::CastMouseRay()
{
	return H2M::RayH2M();
}

void EnvMapVulkanEditorLayer::OnScenePlay()
{
}

void EnvMapVulkanEditorLayer::OnSceneStop()
{
}

void EnvMapVulkanEditorLayer::OnSceneStartSimulation()
{
}

void EnvMapVulkanEditorLayer::OnSceneStopSimulation()
{
}

void EnvMapVulkanEditorLayer::OnSceneTransition(const std::string& scene)
{
}

void EnvMapVulkanEditorLayer::UpdateWindowTitle(const std::string& sceneName)
{
}

void EnvMapVulkanEditorLayer::UI_DrawMenubar()
{
}

float EnvMapVulkanEditorLayer::UI_DrawTitlebar()
{
	return 0.0f;
}

void EnvMapVulkanEditorLayer::UI_HandleManualWindowResize()
{
}

bool EnvMapVulkanEditorLayer::UI_TitleBarHitTest(int x, int y) const
{
	return false;
}

float EnvMapVulkanEditorLayer::GetSnapValue()
{
	return 0.0f;
}

void EnvMapVulkanEditorLayer::DeleteEntity(H2M::EntityH2M entity)
{
}

void EnvMapVulkanEditorLayer::UpdateSceneRendererSettings()
{
}

void EnvMapVulkanEditorLayer::QueueSceneTransition(const std::string& scene)
{
}
