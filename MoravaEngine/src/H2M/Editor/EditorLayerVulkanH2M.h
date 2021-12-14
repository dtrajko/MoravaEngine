/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/Events/KeyEventH2M.h"
#include "H2M/Core/Events/MouseEventH2M.h"
#include "H2M/Core/LayerH2M.h"
#include "H2M/Core/Math/RayH2M.h"
#include "H2M/Editor/SceneHierarchyPanelH2M.h"
#include "H2M/Project/UserPreferencesH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/SceneRendererVulkanH2M.h"
#include "H2M/Scene/EntityH2M.h"


namespace H2M
{

	class EditorLayerVulkanH2M : public LayerH2M
	{
	public:
		enum class PropertyFlag
		{
			None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
		};

	public:
		EditorLayerVulkanH2M(const RefH2M<UserPreferencesH2M>& userPreferences);
		virtual ~EditorLayerVulkanH2M() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(TimestepH2M ts) override;

		virtual void OnImGuiRender() override;
		virtual void OnEvent(EventH2M& e) override;

		virtual void OnRender() override; // this method seems to be obsolete

		void OnRender2D();

		bool OnKeyPressedEvent(KeyPressedEventH2M& e);
		bool OnMouseButtonPressed(MouseButtonPressedEventH2M& e);

		void SelectEntity(EntityH2M entity);

		void OpenProject();
		void OpenProject(const std::string& filepath);
		void CreateProject(std::filesystem::path projectPath);
		void SaveProject();
		void CloseProject(bool unloadProject = true);

		void NewScene(const std::string& name = "UntitledScene");
		void OpenScene();
		void OpenScene(const std::string& filepath);
		void SaveScene();
		void SaveSceneAs();

		void UI_WelcomePopup();
		void UI_AboutPopup();

		void UI_CreateNewMeshPopup();
		void UI_InvalidAssetMetadataPopup();

		void UI_NewScene();

		void SceneHierarchyInvalidMetadataCallback(EntityH2M entity, AssetHandleH2M handle);

	private:
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCameraH2M& camera, float mx, float my);

		struct SelectedSubmesh
		{
			EntityH2M entity;
			SubmeshH2M* Mesh = nullptr;
			float Distance = 0.0f;
		};

		void OnSelected(const SelectedSubmesh& selectionContext);
		void OnEntityDeleted(EntityH2M e);
		RayH2M CastMouseRay();

		void OnScenePlay();
		void OnSceneStop();
		void OnSceneStartSimulation();
		void OnSceneStopSimulation();

		void UpdateWindowTitle(const std::string& sceneName);
		void UI_DrawMenubar();
		// Returns titlebar height
		float UI_DrawTitlebar();
		void UI_HandleManualWindowResize();
		bool UI_TitleBarHitTest(int x, int y) const;

		float GetSnapValue();

		void DeleteEntity(EntityH2M entity);

		void UpdateSceneRendererSettings();

	private:
		RefH2M<UserPreferencesH2M> m_UserPreferences;
		ScopeH2M<SceneHierarchyPanelH2M> m_SceneHierarchyPanel, m_PrefabHierarchyPanel;

		RefH2M<SceneH2M> m_RuntimeScene, m_EditorScene, m_SimulationScene, m_CurrentScene;
		RefH2M<SceneRendererVulkanH2M> m_ViewportRenderer;
		RefH2M<SceneRendererVulkanH2M> m_SecondViewportRenderer;
		RefH2M<SceneRendererVulkanH2M> m_FocusedRenderer;
		RefH2M<Renderer2D_H2M> m_Renderer2D;
		std::string m_SceneFilePath;

		EditorCameraH2M m_EditorCamera;
		EditorCameraH2M m_SecondEditorCamera;

		RefH2M<ShaderH2M> m_BrushShader;
		RefH2M<MaterialH2M> m_SphereBaseMaterial;

		float m_LineWidth = 2.0f;

		bool m_TitleBarHovered = false;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
			RefH2M<Texture2D_H2M> TextureMap;
			bool SRGB = true;
			bool UseTexture = false;
		};
		// AlbedoInput m_AlbedoInput;

		struct NormalInput
		{
			RefH2M<Texture2D_H2M> TextureMap;
			bool UseTexture = false;
		};
		// NormalInput m_NormalInput;

		struct MetalnessInput
		{
			float Value = 1.0f;
			RefH2M<Texture2D_H2M> TextureMap;
			bool UseTexture = false;
		};
		// MetalnessInput m_MetalnessInput;

		struct RoughnessInput
		{
			float Value = 0.2f;
			RefH2M<Texture2D_H2M> TextureMap;
			bool UseTexture = false;
		};
		// RoughnessInput m_RoughnessInput;

		float m_EnvMapRotation = 0.0f;

		// Editor resources
		RefH2M<Texture2D_H2M> m_CheckerboardTex;
		RefH2M<Texture2D_H2M> m_PlayButtonTex, m_StopButtonTex, m_PauseButtonTex, m_SimulateButtonTex;
		RefH2M<Texture2D_H2M> m_SelectToolTex, m_MoveToolTex, m_RotateToolTex, m_ScaleToolTex;
		RefH2M<Texture2D_H2M> m_IconMinimize, m_IconMaximize, m_IconRestore, m_IconClose;
		RefH2M<Texture2D_H2M> m_LogoTex;

		// Icons
		RefH2M<Texture2D_H2M> m_PointLightIcon;

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

		bool m_ShowIcons = true;

		bool m_ViewportPanelMouseOver = false;
		bool m_ViewportPanelFocused = false;
		bool m_AllowViewportCameraEvents = false;

		bool m_ShowConsolePanel = true;

		bool m_ViewportPanel2MouseOver = false;
		bool m_ViewportPanel2Focused = false;

		bool m_ShowProjectSettings = false;
		bool m_ShowSecondViewport = false;
		bool m_ShowAudioEventsEditor = false;

		bool m_ShowWelcomePopup = true;
		bool m_ShowAboutPopup = false;
		bool m_ShowCreateNewProjectPopup = false;

		bool m_ShowNewScenePopup = false;

		bool m_ShowCreateNewMeshPopup = false;

		struct CreateNewMeshPopupData
		{
			std::array<char, 256> CreateMeshFilenameBuffer;
			EntityH2M TargetEntity;

			CreateNewMeshPopupData()
			{
				CreateMeshFilenameBuffer.fill(0);
				TargetEntity = {};
			}
		} m_CreateNewMeshPopupData;

		bool m_ShowInvalidAssetMetadataPopup = false;
		struct InvalidAssetMetadataPopupData
		{
		} m_InvalidAssetMetadataPopupData;

		enum class SceneState
		{
			Edit = 0, Play = 1, Pause = 2, Simulate = 3
		};
		SceneState m_SceneState = SceneState::Edit;

		enum class SelectionMode
		{
			None = 0, Entity = 1, Submesh = 2
		};

		bool m_AssetManagerPanelOpen = false;

		SelectionMode m_SelectionMode = SelectionMode::Entity;
		std::vector<SelectedSubmesh> m_SelectionContext;
		glm::mat4* m_RelativeTransform = nullptr;
		glm::mat4* m_CurrentlySelectedTransform = nullptr;

	};

}
