#pragma once

#include "../../pch.h"

#include "H2M/Editor/ContentBrowserPanelH2M.h"
#include "H2M/Editor/SceneHierarchyPanelH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "Editor/EntitySelection.h"
#include "Editor/MaterialEditorPanel.h"
#include "EnvMap/EnvMapMaterial.h"
#include "EnvMap/EnvMapSharedData.h"
#include "Framebuffer/ShadowMap.h"


enum class SelectionMode
{
	None = 0,
	Entity = 1,
	SubMesh = 2,
};

using SubmeshUUID = std::string;

class EnvMapEditorLayer
{
public:
	EnvMapEditorLayer() = default;
	EnvMapEditorLayer(const std::string& filepath, Scene* scene);
	~EnvMapEditorLayer();

	void Init();

	void OnAttach();
	void OnDetach();

	void OnUpdate(float timestep);
	void OnUpdateEditor(H2M::RefH2M<H2M::SceneH2M> scene, float timestep);
	void OnUpdateRuntime(H2M::RefH2M<H2M::SceneH2M> scene, float timestep);

	void OnRenderShadow(Window* mainWindow);
	void RenderSubmeshesShadowPass(H2M::RefH2M<MoravaShader> shader);

	void OnRenderShadowOmni(Window* mainWindow);
	void RenderShadowOmniSingleLight(Window* mainWindow, H2M::EntityH2M lightEntity, H2M::RefH2M<OmniShadowMap> omniShadowMap);

	void OnRenderCascadedShadowMaps(Window* mainWindow);

	void OnRenderWaterReflection(Window* mainWindow);
	void OnRenderWaterRefraction(Window* mainWindow);

	void OnRender(Window* mainWindow);

	void PostProcessing(Window* mainWindow);

	// void RenderFullscreen(Window* mainWindow);
	// bool GetFullscreenEnabled() { return m_FullscreenEnabled; }

	void OnRenderEditor();
	void OnRenderRuntime();

	void OnImGuiRender(Window* mainWindow, Scene* scene);
	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);

	void DisplaySubmeshMaterialSelector(bool* p_open);

	void UpdateSubmeshMaterialMap(H2M::EntityH2M entity, H2M::SubmeshH2M* submesh);

	// EditorLayer
	void OnEvent(H2M::EventH2M& e);

	bool OnKeyPressedEvent(H2M::KeyPressedEventH2M& e); // EditorLayer::OnKeyPressedEvent()
	bool OnMouseButtonPressed(H2M::MouseButtonPressedEventH2M& e); // EditorLayer::OnMouseButtonPressedEvent()
	bool OnMouseScrolled(H2M::MouseScrolledEventH2M& e);

	void OnOverlayRender();

	void NewScene();
	void OpenScene();
	void OpenScene(const std::filesystem::path& path);
	void SaveScene();
	void SaveSceneAs();

	void SerializeScene(H2M::RefH2M<H2M::SceneH2M> scene, const std::filesystem::path& path);

	void OnScenePlay();
	void OnSceneStop();

	void OnDuplicateEntity();

	// UI Panels
	void UI_Toolbar();

	void OnNewScene(glm::vec2 viewportSize);

	void UpdateWindowTitle(const std::string& sceneName);

	void OnSelected(const SelectedSubmesh& selectionContext);
	void OnEntityDeleted(H2M::EntityH2M e);

	void SelectEntity(H2M::EntityH2M e);

	void CameraSyncECS();
	void UpdateImGuizmo(Window* mainWindow);
	H2M::EntityH2M CreateEntity(const std::string& name);
	H2M::EntityH2M LoadEntity(std::string fullPath);
	H2M::CameraComponentH2M GetMainCameraComponent();

	void ShowBoundingBoxes(bool showBoundingBoxes, bool showBoundingBoxesOnTop);

	static void AddSubmeshToSelectionContext(SelectedSubmesh submesh);

	// Setters
	void SetSkyboxLOD(float LOD);

	// Getters
	H2M::RefH2M<MoravaShader> GetShaderPBR_Anim();
	H2M::RefH2M<MoravaShader> GetShaderPBR_Static();
	inline std::map<std::string, unsigned int>& GetSamplerSlots() { return EnvMapSharedData::s_SamplerSlots; }
	inline bool& GetRadiancePrefilter() { return EnvMapSharedData::s_RadiancePrefilter; }
	inline float& GetEnvMapRotation() { return EnvMapSharedData::s_EnvMapRotation; }
	inline H2M::RefH2M<H2M::Texture2D_H2M> GetCheckerboardTexture() { return s_CheckerboardTexture; }
	inline H2M::RefH2M<H2M::TextureCubeH2M> GetSkyboxTexture() { return m_SkyboxTexture; }
	H2M::EntityH2M GetMeshEntity();
	inline float& GetSkyboxExposureFactor() { return EnvMapSharedData::s_SkyboxExposureFactor; };
	float& GetSkyboxLOD();
	void SetViewportBounds(glm::vec2* viewportBounds);
	inline bool* GetDisplayHazelGrid() { return &EnvMapSharedData::s_DisplayHazelGrid; }
	inline bool* GetDisplayBoundingBoxes() { return &m_DisplayBoundingBoxes; };
	inline bool* GetDisplayRay() { return &EnvMapSharedData::s_DisplayRay; };

	// Renderer
	void SubmitMesh(H2M::MeshH2M* mesh, const glm::mat4& transform, Material* overrideMaterial);

	// from SceneHazelEnvMap
	void SetupRenderFramebuffer();
	void ResizeViewport();

	H2M::CameraH2M* GetActiveCamera() const { return m_ActiveCamera; }

	H2M::RefH2M<H2M::SceneH2M> GetActiveScene() { return m_ActiveScene; }

private:
	void SetupContextData(Scene* scene);
	void SetupShaders();
	void UpdateUniforms();
	void SetSkybox(H2M::RefH2M<H2M::TextureCubeH2M> skybox);
	void SetupLights(); // temporary solution until lights are properly deserialized

	std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my); // EditorLayer::CastRay()
	std::pair<float, float> GetMouseViewportSpace();
	std::vector<glm::mat4> CalculateLightTransform(glm::mat4 lightProj, glm::vec3 position);
	std::pair<int, int> GetMouseCoordsInViewport();
	std::pair<int, int> GetMouseCoordsInViewportFlipY();

	std::vector<H2M::EntityH2M> GetDirectionalLightEntities();
	std::vector<H2M::EntityH2M> GetPointLightEntities();
	std::vector<H2M::EntityH2M> GetSpotLightEntities();

public:
	std::string m_Filepath;
	Scene* m_Scene;

	static SelectionMode s_SelectionMode;
	static H2M::RefH2M<H2M::Texture2D_H2M> s_CheckerboardTexture;

	static H2M::RefH2M<EnvMapMaterial> s_DefaultMaterial;
	static H2M::RefH2M<EnvMapMaterial> s_LightMaterial;

	glm::mat4 m_CurrentlySelectedTransform;
	glm::mat4* m_RelativeTransform = nullptr;
	bool m_AllowViewportCameraEvents = true; // EditorLayer (Raypicking)

	// BEGIN Render Framebuffers
	H2M::RefH2M<MoravaFramebuffer> m_RenderFramebuffer;
	H2M::RefH2M<MoravaFramebuffer> m_PostProcessingFramebuffer;

	H2M::RefH2M<H2M::FramebufferH2M> m_RenderFramebufferTempH2M;
	// END Render Framebuffers

	// BEGIN Viewport
	struct Viewport
	{
		int X;
		int Y;
		int Width;
		int Height;
		int MouseX;
		int MouseY;
	};

	Viewport m_ImGuiViewport;
	Viewport m_ImGuiViewportEnvMap;

	// viewports private
	// -- viewport main
	// -- viewport environment map
	int m_ImGuiViewportEnvMapX;
	int m_ImGuiViewportEnvMapY;
	bool m_IsViewportEnvMapEnabled;
	bool m_ViewportEnvMapFocused;
	bool m_ViewportEnvMapHovered;
	glm::vec2 m_ViewportEnvMapSize;

	bool m_ViewportFocused = false;
	bool m_ViewportHovered = false;

	// Used in EnvMapEditorLayer::CastRay
	glm::vec2 m_ViewportBounds[2];
	glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	glm::vec2 m_ViewportSizePrevious = { 0.0f, 0.0f };

	EventCooldown m_ResizeViewport;

	glm::vec2 m_ImGuiViewportMain;

	std::pair<uint32_t, uint32_t> m_MouseCoordsInViewport = { 0, 0 };
	std::pair<uint32_t, uint32_t> m_MouseCoordsInViewportFlipY = { 0, 0 };
	// END Viewport

	int m_EntityID = -1;

private:
	H2M::RefH2M<MoravaShader> m_ShaderShadow;
	H2M::RefH2M<MoravaShader> m_ShaderOmniShadow;
	H2M::RefH2M<MoravaShader> m_ShaderPostProcessing;
	H2M::RefH2M<MoravaShader> m_ShaderBloomBlur;

	int m_PostProcessingEffect = 0;
	bool m_PostProcessingEnabled = false;

	H2M::RefH2M<H2M::TextureCubeH2M> m_SkyboxTexture;

	// H2M::EntityH2M m_DirectionalLightEntity;
	glm::mat4 m_LightProjectionMatrix;
	glm::vec3 m_LightDirection; // temporary, use DirectionalLightComponent
	/** END properties Hazelnut/EditorLayer **/

	// Materials
	float m_MaterialSpecular = 0.0f;
	float m_MaterialShininess = 0.0f;

	glm::vec3 m_NewDir;

	glm::vec2 m_WorkPosImGui; // window offset on monitor real estate

	enum class SceneState
	{
		Edit = 0,
		Play,
		Pause,
		Simulate,
	};
	SceneState m_SceneState = SceneState::Edit;

	bool m_DisplayBoundingBoxes;
	bool m_DrawOnTopBoundingBoxes; // obsolete?
	bool m_DisplayLineElements;

	// Hazel LIVE! #014
	bool m_ReloadScriptOnPlay = true;

	// Panels
	H2M::SceneHierarchyPanelH2M* m_SceneHierarchyPanel;
	H2M::ContentBrowserPanelH2M* m_ContentBrowserPanel;
	MaterialEditorPanel* m_MaterialEditorPanel;

	// Editor resources (BEGIN properties Hazelnut/EditorLayer)
	H2M::RefH2M<H2M::Texture2D_H2M> m_PlayButtonTex;

	H2M::RefH2M<H2M::Texture2D_H2M> m_IconPlay;
	H2M::RefH2M<H2M::Texture2D_H2M> m_IconStop;

	H2M::EditorCameraH2M* m_EditorCamera;
	RuntimeCamera* m_RuntimeCamera;
	H2M::CameraH2M* m_ActiveCamera;

	H2M::RefH2M<H2M::SceneH2M> m_ActiveScene;
	H2M::RefH2M<H2M::SceneH2M> m_EditorScene;

	std::filesystem::path m_EditorScenePath;

	float m_CameraMoveSpeed = 2.0f;

	// Hazel LIVE! #015
	bool m_UIShowBoundingBoxes;
	bool m_UIShowBoundingBoxesOnTop;

	std::string m_WindowTitleStatic = "";
	std::string m_WindowTitleDynamic = "";

	std::string m_StatusBarMessage = "";

	// bool m_FullscreenEnabled = false;

	bool m_ShowWindowSceneHierarchy = true;
	bool m_ShowWindowTransform      = true;
	bool m_ShowWindowMaterialEditor = true;
	bool m_ShowWindowAssetManager   = true;

	bool m_ShowWindowPostProcessing = false;
	bool m_ShowWindowShaderManager  = false;
	bool m_ShowWindowFramebuffers   = false;
	bool m_ShowWindowSettings       = false;
	bool m_ShowWindowRendererStats  = false;
	bool m_ShowWindowHelp           = false;
	bool m_ShowWindowMousePicker    = false;
	bool m_ShowWindowViewportInfo   = false;
	bool m_ShowWindowImGuiMetrics   = false;
	bool m_ShowViewportBounds       = true;

	std::string m_EnvMapFilename = "File Path";

	friend class EnvMapSceneRenderer;

};
