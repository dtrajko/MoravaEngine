#pragma once

#include "../../pch.h"

#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Editor/ContentBrowserPanel.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "Editor/EntitySelection.h"
#include "Editor/MaterialEditorPanel.h"
#include "EnvMap/EnvMapMaterial.h"
#include "EnvMap/EnvMapSceneRenderer.h"
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

	void OnUpdate(float timestep);
	void OnUpdateEditor(Hazel::Ref<Hazel::HazelScene> scene, float timestep);
	void OnUpdateRuntime(Hazel::Ref<Hazel::HazelScene> scene, float timestep);

	void OnScenePlay();
	void OnSceneStop();

	void OnRenderShadow(Window* mainWindow);
	void RenderSubmeshesShadowPass(Hazel::Ref<MoravaShader> shader);

	void OnRenderShadowOmni(Window* mainWindow);
	void RenderShadowOmniSingleLight(Window* mainWindow, Hazel::Entity lightEntity, Hazel::Ref<OmniShadowMap> omniShadowMap);

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

	void UpdateSubmeshMaterialMap(Hazel::Entity entity, Hazel::SubmeshHazelLegacy* submesh);

	void NewScene();
	void OpenScene();
	void SaveScene();
	void SaveSceneAs();

	void OnNewScene(glm::vec2 viewportSize);

	void UpdateWindowTitle(const std::string& sceneName);

	void OnSelected(const SelectedSubmesh& selectionContext);
	void OnEntityDeleted(Hazel::Entity e);

	bool OnKeyPressedEvent(KeyPressedEvent& e); // EditorLayer::OnKeyPressedEvent()
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e); // EditorLayer::OnMouseButtonPressedEvent()

	void SelectEntity(Hazel::Entity e);

	void CameraSyncECS();
	void UpdateImGuizmo(Window* mainWindow);
	Hazel::Entity CreateEntity(const std::string& name);
	Hazel::Entity LoadEntity(std::string fullPath);
	static Hazel::CameraComponent GetMainCameraComponent();

	void ShowBoundingBoxes(bool showBoundingBoxes, bool showBoundingBoxesOnTop);

	static void AddSubmeshToSelectionContext(SelectedSubmesh submesh);

	// Setters
	void SetSkyboxLOD(float LOD);

	// Getters
	Hazel::Ref<MoravaShader> GetShaderPBR_Anim();
	Hazel::Ref<MoravaShader> GetShaderPBR_Static();
	inline std::map<std::string, unsigned int>& GetSamplerSlots() { return EnvMapSharedData::s_SamplerSlots; }
	inline bool& GetRadiancePrefilter() { return EnvMapSharedData::s_RadiancePrefilter; }
	inline float& GetEnvMapRotation() { return EnvMapSharedData::s_EnvMapRotation; }
	inline Hazel::Ref<Hazel::HazelTexture2D> GetCheckerboardTexture() { return s_CheckerboardTexture; }
	inline Hazel::Ref<Hazel::HazelTextureCube> GetSkyboxTexture() { return m_SkyboxTexture; }
	Ref<Hazel::Entity> GetMeshEntity();
	inline float& GetSkyboxExposureFactor() { return EnvMapSharedData::s_SkyboxExposureFactor; };
	float& GetSkyboxLOD();
	void SetViewportBounds(glm::vec2* viewportBounds);
	inline bool* GetDisplayHazelGrid() { return &EnvMapSharedData::s_DisplayHazelGrid; }
	inline bool* GetDisplayBoundingBoxes() { return &m_DisplayBoundingBoxes; };
	inline bool* GetDisplayRay() { return &EnvMapSharedData::s_DisplayRay; };

	// Renderer
	void DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest);
	void SubmitMesh(Hazel::MeshHazelLegacy* mesh, const glm::mat4& transform, Material* overrideMaterial);

	// EditorLayer
	void OnEvent(Event& e);

	// from SceneHazelEnvMap
	void SetupRenderFramebuffer();
	void ResizeViewport(glm::vec2 viewportPanelSize, Hazel::Ref<MoravaFramebuffer> renderFramebuffer);

private:
	void SetupContextData(Scene* scene);
	void SetupShaders();
	void UpdateUniforms();
	void SetSkybox(Hazel::Ref<Hazel::HazelTextureCube> skybox);
	void Init();

	std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my); // EditorLayer::CastRay()
	std::pair<float, float> GetMouseViewportSpace();
	std::vector<glm::mat4> CalculateLightTransform(glm::mat4 lightProj, glm::vec3 position);

public:
	static SelectionMode s_SelectionMode;
	static Hazel::Ref<Hazel::HazelTexture2D> s_CheckerboardTexture;

	static Hazel::Ref<EnvMapMaterial> s_DefaultMaterial;
	static Hazel::Ref<EnvMapMaterial> s_LightMaterial;

	glm::mat4 m_CurrentlySelectedTransform;
	glm::mat4* m_RelativeTransform = nullptr;
	bool m_AllowViewportCameraEvents = true; // EditorLayer (Raypicking)

	// viewports public
	glm::vec2 m_ImGuiViewportMain;
	glm::vec2 m_ViewportMainSize;
	Hazel::Ref<MoravaFramebuffer> m_RenderFramebuffer;
	Hazel::Ref<MoravaFramebuffer> m_PostProcessingFramebuffer;

private:
	Hazel::Ref<MoravaShader> m_ShaderShadow;
	Hazel::Ref<MoravaShader> m_ShaderOmniShadow;
	Hazel::Ref<MoravaShader> m_ShaderPostProcessing;
	Hazel::Ref<MoravaShader> m_ShaderBloomBlur;

	int m_PostProcessingEffect = 0;
	bool m_PostProcessingEnabled = false;

	Hazel::Ref<Hazel::HazelTextureCube> m_SkyboxTexture;

	/** BEGIN properties Hazelnut/EditorLayer **/
	// Editor resources
	Hazel::Ref<Hazel::HazelTexture2D> m_PlayButtonTex;

	Hazel::Entity m_DirectionalLightEntity;
	glm::mat4 m_LightProjectionMatrix;
	glm::vec3 m_LightDirection; // temporary, use DirectionalLightComponent

	float m_ViewportWidth = 0.0f;
	float m_ViewportHeight = 0.0f;
	/** END properties Hazelnut/EditorLayer **/

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

	bool m_ViewportPanelMouseOver = false;
	bool m_ViewportPanelFocused = false;

	// Used in EnvMapEditorLayer::CastRay
	glm::vec2 m_ViewportBounds[2];

	EventCooldown m_ResizeViewport;

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
	};
	SceneState m_SceneState;

	bool m_DisplayBoundingBoxes;
	bool m_DrawOnTopBoundingBoxes; // obsolete?
	bool m_DisplayLineElements;

	// Hazel LIVE! #014
	std::string m_SceneFilePath;
	bool m_ReloadScriptOnPlay = true;

	Hazel::SceneHierarchyPanel* m_SceneHierarchyPanel;
	Hazel::ContentBrowserPanel* m_ContentBrowserPanel;
	MaterialEditorPanel* m_MaterialEditorPanel;

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

	std::string m_EnvMapFilename = "File Path";

};
