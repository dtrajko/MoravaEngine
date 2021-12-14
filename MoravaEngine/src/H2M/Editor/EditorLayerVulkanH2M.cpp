/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EditorLayerVulkanH2M.h"

#include "H2M/Renderer/Renderer2D_H2M.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "ImGuizmo.h"


namespace H2M
{

#define MAX_PROJECT_NAME_LENGTH 255
#define MAX_PROJECT_FILEPATH_LENGTH 512

	static char* s_ProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
	static char* s_ProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];


	EditorLayerVulkanH2M::EditorLayerVulkanH2M(const RefH2M<UserPreferencesH2M>& userPreferences)
		: m_UserPreferences(userPreferences)
	{
		Log::GetLogger()->info("EditorLayerVulkanH2M::EditorLayerVulkan method called!");
	}

	EditorLayerVulkanH2M::~EditorLayerVulkanH2M()
	{
		Log::GetLogger()->info("EditorLayerVulkanH2M::~EditorLayerVulkan method called!");
	}

	auto operator < (const ImVec2& lhs, const ImVec2 rhs)
	{
		return lhs.x < rhs.x&& lhs.y < rhs.y;
	}

	static std::string s_HazelInstallPath = "";

	void EditorLayerVulkanH2M::OnAttach()
	{
		Log::GetLogger()->info("EditorLayerVulkanH2M::OnAttach method called!");
	}

	void EditorLayerVulkanH2M::OnDetach()
	{
		Log::GetLogger()->info("EditorLayerVulkanH2M::OnAttach method called!");
	}

	void EditorLayerVulkanH2M::OnScenePlay()
	{
	}

	void EditorLayerVulkanH2M::OnSceneStop()
	{
	}

	void EditorLayerVulkanH2M::OnSceneStartSimulation()
	{
	}

	void EditorLayerVulkanH2M::OnSceneStopSimulation()
	{
	}

	void EditorLayerVulkanH2M::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string rendererAPI = "";
		
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::OpenGL:
				rendererAPI = "OpenGL";
				break;
			case RendererAPITypeH2M::Vulkan:
				rendererAPI = "Vulkan";
				break;
			case RendererAPITypeH2M::DX11:
				rendererAPI = "DirectX 11";
				break;
		}
	}

	void EditorLayerVulkanH2M::UI_DrawMenubar()
	{
	}

	float EditorLayerVulkanH2M::UI_DrawTitlebar()
	{
		return 0.0f;
	}

	void EditorLayerVulkanH2M::UI_HandleManualWindowResize()
	{
	}

	bool EditorLayerVulkanH2M::UI_TitleBarHitTest(int x, int y) const
	{
		return m_TitleBarHovered;
	}

	float EditorLayerVulkanH2M::GetSnapValue()
	{
		switch (m_GizmoType)
		{
			case ImGuizmo::OPERATION::TRANSLATE: return 0.5f;
			case ImGuizmo::OPERATION::ROTATE: return 45.0f;
			case ImGuizmo::OPERATION::SCALE: return 0.5f;
		}
		return 0.0f;
	}

	void EditorLayerVulkanH2M::DeleteEntity(EntityH2M entity)
	{
	}

	void EditorLayerVulkanH2M::UpdateSceneRendererSettings()
	{
	}

	void EditorLayerVulkanH2M::OnUpdate(TimestepH2M ts)
	{
	}

	void EditorLayerVulkanH2M::OnImGuiRender()
	{
	}

	void EditorLayerVulkanH2M::OnEvent(EventH2M& e)
	{
	}

	// this method seems to be obsolete
	void EditorLayerVulkanH2M::OnRender()
	{
	}

	void EditorLayerVulkanH2M::OnRender2D()
	{
	}

	bool EditorLayerVulkanH2M::OnKeyPressedEvent(KeyPressedEventH2M& e)
	{
		return false;
	}

	bool EditorLayerVulkanH2M::OnMouseButtonPressed(MouseButtonPressedEventH2M& e)
	{
		return false;
	}

	void EditorLayerVulkanH2M::SelectEntity(EntityH2M entity)
	{
	}

	void EditorLayerVulkanH2M::OpenProject()
	{
	}

	void EditorLayerVulkanH2M::OpenProject(const std::string& filepath)
	{
	}

	void EditorLayerVulkanH2M::CreateProject(std::filesystem::path projectPath)
	{
	}

	void EditorLayerVulkanH2M::SaveProject()
	{
	}

	void EditorLayerVulkanH2M::CloseProject(bool unloadProject)
	{
	}

	void EditorLayerVulkanH2M::NewScene(const std::string& name)
	{
	}

	void EditorLayerVulkanH2M::OpenScene()
	{
	}

	void EditorLayerVulkanH2M::OpenScene(const std::string& filepath)
	{
	}

	void EditorLayerVulkanH2M::SaveScene()
	{
	}

	void EditorLayerVulkanH2M::SaveSceneAs()
	{
	}

	void EditorLayerVulkanH2M::UI_WelcomePopup()
	{
	}

	void EditorLayerVulkanH2M::UI_AboutPopup()
	{
	}

	void EditorLayerVulkanH2M::UI_CreateNewMeshPopup()
	{
	}

	void EditorLayerVulkanH2M::UI_InvalidAssetMetadataPopup()
	{
	}

	void EditorLayerVulkanH2M::UI_NewScene()
	{
	}

	std::pair<float, float> EditorLayerVulkanH2M::GetMouseViewportSpace(bool primaryViewport)
	{
		return std::pair<float, float>();
	}

	std::pair<glm::vec3, glm::vec3> EditorLayerVulkanH2M::CastRay(const EditorCameraH2M& camera, float mx, float my)
	{
		return std::pair<glm::vec3, glm::vec3>();
	}

	void EditorLayerVulkanH2M::SceneHierarchyInvalidMetadataCallback(EntityH2M entity, AssetHandleH2M handle)
	{
	}

	void EditorLayerVulkanH2M::OnSelected(const SelectedSubmesh& selectionContext)
	{
	}

	void EditorLayerVulkanH2M::OnEntityDeleted(EntityH2M e)
	{
	}

	RayH2M EditorLayerVulkanH2M::CastMouseRay()
	{
		return RayH2M();
	}

}
