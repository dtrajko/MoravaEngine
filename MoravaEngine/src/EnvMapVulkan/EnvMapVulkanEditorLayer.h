#pragma once

#include "H2M/Asset/AssetMetadataH2M.h"
#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/Events/KeyEventH2M.h"
#include "H2M/Core/Events/MouseEventH2M.h"
#include "H2M/Core/LayerH2M.h"
#include "H2M/Core/Math/RayH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Core/TimestepH2M.h"
#include "H2M/Editor/PanelManagerH2M.h"
#include "H2M/Project/UserPreferencesH2M.h"
#include "H2M/Scene/EntityH2M.h"

#include "EnvMapVulkan/EnvMapVulkanSceneRenderer.h"


class EnvMapVulkanEditorLayer : public H2M::LayerH2M
{
public:
	enum class PropertyFlag
	{
		None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
	};
public:
	EnvMapVulkanEditorLayer(const H2M::RefH2M<H2M::UserPreferencesH2M>& userPreferences);
	virtual ~EnvMapVulkanEditorLayer() override;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnUpdate(H2M::TimestepH2M ts) override;

	virtual void OnRender() override; // should we remove the pure virtual method LayerH2M::OnRender and why?

	virtual void OnImGuiRender() override;
	virtual void OnEvent(H2M::EventH2M& e) override;

	void OnRender2D();

	bool OnKeyPressedEvent(H2M::KeyPressedEventH2M& e);
	bool OnMouseButtonPressed(H2M::MouseButtonPressedEventH2M& e);

	void SelectEntity(H2M::EntityH2M entity);

	void OpenProject();
	void OpenProject(const std::string& filepath);
	void CreateProject(std::filesystem::path projectPath);
	void SaveProject();
	void CloseProject(bool unloadProject = true);
	void NewScene(const std::string& name = "UntitledScene");
	void OpenScene();
	void OpenScene(const std::string& filepath);
	void OpenScene(const H2M::AssetMetadataH2M& assetMetadata);
	void SaveScene();
	void SaveSceneAs();

	void UI_WelcomePopup();
	void UI_AboutPopup();

	void UI_CreateNewMeshPopup();
	void UI_InvalidAssetMetadataPopup();

	void UI_NewScene();

	void SceneHierarchyInvalidMetadataCallback(H2M::EntityH2M entity, H2M::AssetHandleH2M handle);
private:
	std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);
	std::pair<glm::vec3, glm::vec3> CastRay(const H2M::EditorCameraH2M& camera, float mx, float my);

	std::vector<std::function<void()>> m_PostSceneUpdateQueue;

	struct SelectedSubmesh
	{
		H2M::EntityH2M Entity;
		H2M::SubmeshH2M* Mesh = nullptr;
		float Distance = 0.0f;
	};

	void OnSelected(const SelectedSubmesh& selectionContext);
	void OnEntityDeleted(H2M::EntityH2M e);
	// void OnCreateMeshFromMeshSource(H2M::EntityH2M entity, H2M::RefH2M<H2M::MeshSourceH2M> meshSource);
	void OnCreateMeshFromMeshSource(H2M::EntityH2M entity, H2M::RefH2M<H2M::MeshH2M> mesh);

	H2M::RayH2M CastMouseRay();

	void OnScenePlay();
	void OnSceneStop();
	void OnSceneStartSimulation();
	void OnSceneStopSimulation();

	void OnSceneTransition(const std::string& scene);

	void UpdateWindowTitle(const std::string& sceneName);
	void UI_DrawMenubar();
	// Returns titlebar height
	float UI_DrawTitlebar();
	void UI_HandleManualWindowResize();
	bool UI_TitleBarHitTest(int x, int y) const;

	float GetSnapValue();

	void DeleteEntity(H2M::EntityH2M entity);

	void UpdateSceneRendererSettings();
	void QueueSceneTransition(const std::string& scene);
private:
	H2M::RefH2M<H2M::UserPreferencesH2M> m_UserPreferences;

	ScopeH2M<H2M::PanelManagerH2M> m_PanelManager;
	bool m_ShowPhysicsPanel = true;

	H2M::RefH2M<H2M::SceneH2M> m_RuntimeScene, m_EditorScene, m_SimulationScene, m_CurrentScene;
	H2M::RefH2M<EnvMapVulkanSceneRenderer> m_ViewportRenderer;
	H2M::RefH2M<EnvMapVulkanSceneRenderer> m_SecondViewportRenderer;
	H2M::RefH2M<EnvMapVulkanSceneRenderer> m_FocusedRenderer;
	H2M::RefH2M<H2M::Renderer2D_H2M> m_Renderer2D;
	std::string m_SceneFilePath;

	H2M::EditorCameraH2M m_EditorCamera;
	H2M::EditorCameraH2M m_SecondEditorCamera;

	H2M::RefH2M<H2M::ShaderH2M> m_BrushShader;
	H2M::RefH2M<H2M::MaterialH2M> m_SphereBaseMaterial;

	float m_LineWidth = 2.0f;

	bool m_TitleBarHovered = false;

	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	//AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	//NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	//MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.2f;
		H2M::RefH2M<H2M::Texture2D_H2M> TextureMap;
		bool UseTexture = false;
	};
	//RoughnessInput m_RoughnessInput;

	float m_EnvMapRotation = 0.0f;

	// Editor resources
	H2M::RefH2M<H2M::Texture2D_H2M> m_CheckerboardTex;
	H2M::RefH2M<H2M::Texture2D_H2M> m_PlayButtonTex, m_StopButtonTex, m_PauseButtonTex, m_SimulateButtonTex;
	H2M::RefH2M<H2M::Texture2D_H2M> m_SelectToolTex, m_MoveToolTex, m_RotateToolTex, m_ScaleToolTex;
	H2M::RefH2M<H2M::Texture2D_H2M> m_IconMinimize, m_IconMaximize, m_IconRestore, m_IconClose;
	H2M::RefH2M<H2M::Texture2D_H2M> m_LogoTex;

	// Icons
	H2M::RefH2M<H2M::Texture2D_H2M> m_PointLightIcon;

	glm::vec2 m_ViewportBounds[2];
	glm::vec2 m_SecondViewportBounds[2];
	int m_GizmoType = -1; // -1 = no gizmo
	float m_SnapValue = 0.5f;
	float m_RotationSnapValue = 45.0f;
	bool m_DrawOnTopBoundingBoxes = true;

	bool m_ShowBoundingBoxes = false;
	bool m_ShowBoundingBoxSelectedMeshOnly = true;
	bool m_ShowBoundingBoxSubmeshes = false;
	bool m_ShowSelectedWireframe = false;
	bool m_ShowPhysicsCollidersWireframe = false;
	bool m_ShowPhysicsCollidersWireframeOnTop = false;

	bool m_ShowAudioEventsEditor = false;
	bool m_AssetManagerPanelOpen = false;

	bool m_ShowIcons = true;

	bool m_ViewportPanelMouseOver = false;
	bool m_ViewportPanelFocused = false;
	bool m_AllowViewportCameraEvents = false;

	bool m_ViewportPanel2MouseOver = false;
	bool m_ViewportPanel2Focused = false;

	bool m_ShowSecondViewport = false;

	bool m_ShowWelcomePopup = true;
	bool m_ShowAboutPopup = false;
	bool m_ShowCreateNewProjectPopup = false;

	bool m_ShowNewScenePopup = false;

	bool m_ShowCreateNewMeshPopup = false;
	struct CreateNewMeshPopupData
	{
		H2M::RefH2M<H2M::MeshH2M> MeshToCreate; // TODO: MeshH2M => MeshSourceH2M
		std::array<char, 256> CreateMeshFilenameBuffer;
		H2M::EntityH2M TargetEntity;

		CreateNewMeshPopupData()
		{
			CreateMeshFilenameBuffer.fill(0);
			MeshToCreate = nullptr;
			TargetEntity = {};
		}

	} m_CreateNewMeshPopupData;

	bool m_ShowInvalidAssetMetadataPopup = false;
	struct InvalidAssetMetadataPopupData
	{
		H2M::AssetMetadataH2M Metadata;
	} m_InvalidAssetMetadataPopupData;

	enum class SceneState
	{
		Edit = 0, Play = 1, Pause = 2, Simulate = 3
	};
	SceneState m_SceneState = SceneState::Edit;

	enum class SelectionMode
	{
		None = 0, Entity = 1, SubMesh = 2
	};

	SelectionMode m_SelectionMode = SelectionMode::Entity;
	std::vector<SelectedSubmesh> m_SelectionContext;
	glm::mat4* m_RelativeTransform = nullptr;
	glm::mat4* m_CurrentlySelectedTransform = nullptr;
};
