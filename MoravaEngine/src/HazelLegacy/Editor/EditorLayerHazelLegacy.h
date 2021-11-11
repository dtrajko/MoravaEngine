#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Asset/AssetMetadata.h"
#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Math/Ray.h"
#include "Hazel/Editor/ContentBrowserPanel.h"
#include "Hazel/Editor/EditorConsolePanel.h"
#include "Hazel/Editor/ProjectSettingsWindow.h"
#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Core/Events/MouseEvent.h"
#include "Hazel/Project/UserPreferences.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "HazelLegacy/Renderer/SceneRendererHazelLegacy.h"


namespace Hazel
{

	class EditorLayerHazelLegacy : public Layer
	{
	public:
		enum class PropertyFlag
		{
			None = 0, ColorProperty = 1, DragProperty = 2, SliderProperty = 4
		};

	public:
		EditorLayerHazelLegacy(const Ref<UserPreferences>& userPreferences);
		virtual ~EditorLayerHazelLegacy() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& e) override;

		virtual void OnRender() override; // this method seems to be obsolete

		void OnRender2D();

		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void SelectEntity(Entity entity);

		void OpenProject();
		void OpenProject(const std::string& filepath);
		void CreateProject(std::filesystem::path projectPath);
		void SaveProject();
		void CloseProject(bool unloadProject = true);

		void NewScene(const std::string& name = "UntitledScene");
		void OpenScene();
		void OpenScene(const std::string& filepath);
		void OpenScene(const AssetMetadata& assetMetadata);
		void SaveScene();
		void SaveSceneAs();

		void UI_WelcomePopup();
		void UI_AboutPopup();

		void UI_CreateNewMeshPopup();
		void UI_InvalidAssetMetadataPopup();

		void UI_NewScene();

		void OnCreateMeshFromMeshAsset(Entity entity, Ref<MeshAsset> meshAsset);
		void SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle);

	private:
		std::pair<float, float> GetMouseViewportSpace(bool primaryViewport);
		std::pair<glm::vec3, glm::vec3> CastRay(const EditorCamera& camera, float mx, float my);

		struct SelectedSubmesh
		{
			Entity entity;
			Submesh* Mesh = nullptr;
			float Distance = 0.0f;
		};

		void OnSelected(const SelectedSubmesh& selectionContext);
		void OnEntityDeleted(Entity e);
		Ray CastMouseRay();

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

		void DeleteEntity(Entity entity);

		void UpdateSceneRendererSettings();

	private:
		Ref<UserPreferences> m_UserPreferences;
		Scope<SceneHierarchyPanel> m_SceneHierarchyPanel, m_PrefabHierarchyPanel;
		Scope<ContentBrowserPanel> m_ContentBrowserPanel;
		Scope<EditorConsolePanel> m_ConsolePanel;
		Scope<ProjectSettingsWindow> m_ProjectSettingsPanel;

		Ref<HazelScene> m_RuntimeScene, m_EditorScene, m_SimulationScene, m_CurrentScene;
		Ref<SceneRendererHazelLegacy> m_ViewportRenderer;
		Ref<SceneRendererHazelLegacy> m_SecondViewportRenderer;
		Ref<SceneRendererHazelLegacy> m_FocusedRenderer;
		Ref<Renderer2D> m_Renderer2D;
		std::string m_SceneFilePath;

		EditorCamera m_EditorCamera;
		EditorCamera m_SecondEditorCamera;

		Ref<HazelShader> m_BrushShader;
		Ref<HazelMaterial> m_SphereBaseMaterial;

		float m_LineWidth = 2.0f;

		bool m_TitleBarHovered = false;

		struct AlbedoInput
		{
			glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
			Ref<HazelTexture2D> TextureMap;
			bool SRGB = true;
			bool UseTexture = false;
		};
		// AlbedoInput m_AlbedoInput;

		struct NormalInput
		{
			Ref<HazelTexture2D> TextureMap;
			bool UseTexture = false;
		};
		// NormalInput m_NormalInput;

		struct MetalnessInput
		{
			float Value = 1.0f;
			Ref<HazelTexture2D> TextureMap;
			bool UseTexture = false;
		};
		// MetalnessInput m_MetalnessInput;

		struct RoughnessInput
		{
			float Value = 0.2f;
			Ref<HazelTexture2D> TextureMap;
			bool UseTexture = false;
		};
		// RoughnessInput m_RoughnessInput;

		float m_EnvMapRotation = 0.0f;

		// Editor resources
		Ref<HazelTexture2D> m_CheckerboardTex;
		Ref<HazelTexture2D> m_PlayButtonTex, m_StopButtonTex, m_PauseButtonTex, m_SimulateButtonTex;
		Ref<HazelTexture2D> m_SelectToolTex, m_MoveToolTex, m_RotateToolTex, m_ScaleToolTex;
		Ref<HazelTexture2D> m_IconMinimize, m_IconMaximize, m_IconRestore, m_IconClose;
		Ref<HazelTexture2D> m_LogoTex;

		// Icons
		Ref<HazelTexture2D> m_PointLightIcon;

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
			Ref<MeshAsset> MeshToCreate;
			std::array<char, 256> CreateMeshFilenameBuffer;
			Entity TargetEntity;

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
			AssetMetadata Metadata;
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
