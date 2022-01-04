#include "EnvMapVulkanEditorLayer.h"

#include "H2M/Renderer/Renderer2D_H2M.h"


EnvMapVulkanEditorLayer::EnvMapVulkanEditorLayer(const H2M::RefH2M<H2M::UserPreferencesH2M>& userPreferences)
{
}

EnvMapVulkanEditorLayer::~EnvMapVulkanEditorLayer()
{
}

void EnvMapVulkanEditorLayer::OnAttach()
{
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
