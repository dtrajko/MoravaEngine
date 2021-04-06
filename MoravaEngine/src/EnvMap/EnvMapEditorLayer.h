#pragma once

#include "../../pch.h"

#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "Editor/EntitySelection.h"
#include "EnvMap/EnvMapMaterial.h"
#include "EnvMap/EnvMapSceneRenderer.h"
#include "Framebuffer/ShadowMap.h"
#include "Framebuffer/OmniShadowMap.h"
#include "Mesh/CubeSkybox.h"
#include "Mesh/Quad.h"


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

	void OnUpdate(Scene* scene, float timestep);
	void OnUpdateEditor(Hazel::Ref<Hazel::HazelScene> scene, float timestep);
	void OnUpdateRuntime(Hazel::Ref<Hazel::HazelScene> scene, float timestep);

	void OnScenePlay();
	void OnSceneStop();

	void OnRender(Framebuffer* framebuffer, Window* mainWindow);
	void OnRenderShadow(Window* mainWindow);
	void OnRenderShadowOmni(Window* mainWindow);

	void RenderShadowOmniSingleLight(Window* mainWindow, Hazel::Entity lightEntity, Hazel::Ref<OmniShadowMap> omniShadowMap);
	void RenderSubmeshesShadowPass(Hazel::Ref<Shader> shader);

	void OnRenderEditor(Framebuffer* framebuffer);
	void OnRenderRuntime(Framebuffer* framebuffer);

	void OnImGuiRender(Window* mainWindow, Scene* scene);
	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);

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
	void ShowBoundingBoxes(bool showBoundingBoxes, bool showBoundingBoxesOnTop);

	static void AddSubmeshToSelectionContext(SelectedSubmesh submesh);

	// Setters
	void SetSkyboxLOD(float LOD);

	// Getters
	Hazel::Ref<Shader> GetShaderPBR_Anim();
	Hazel::Ref<Shader> GetShaderPBR_Static();
	inline std::map<std::string, unsigned int>* GetSamplerSlots() { return m_SamplerSlots; }
	inline bool& GetRadiancePrefilter() { return m_RadiancePrefilter; }
	inline float& GetEnvMapRotation() { return m_EnvMapRotation; }
	inline Hazel::Ref<Hazel::HazelTexture2D> GetCheckerboardTexture() { return s_CheckerboardTexture; }
	inline Hazel::Ref<Hazel::HazelTextureCube> GetSkyboxTexture() { return m_SkyboxTexture; }
	Ref<Hazel::Entity> GetMeshEntity();
	inline float& GetSkyboxExposureFactor() { return m_SkyboxExposureFactor; };
	float& GetSkyboxLOD();
	void SetViewportBounds(glm::vec2* viewportBounds);
	inline bool* GetDisplayHazelGrid() { return &m_DisplayHazelGrid; }
	inline bool* GetDisplayBoundingBoxes() { return &m_DisplayBoundingBoxes; };
	inline bool* GetDisplayRay() { return &m_DisplayRay; };

	// EnvMapSceneRenderer
	void CompositePassTemporary(Framebuffer* framebuffer);
	void GeometryPassTemporary();
	void SubmitEntity(Hazel::Entity entity);

	// Renderer
	void DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest);
	void SubmitMesh(Hazel::HazelMesh* mesh, const glm::mat4& transform, Material* overrideMaterial);

	// EditorLayer
	void OnEvent(Event& e);

	// from SceneHazelEnvMap
	void SetupRenderFramebuffer();
	void ResizeViewport(glm::vec2 viewportPanelSize, Framebuffer* renderFramebuffer);

private:
	void SetupContextData(Scene* scene);
	void SetupShaders();
	void UpdateUniforms();
	void UpdateShaderPBRUniforms(Hazel::Ref<Shader> shaderHazelPBR, Hazel::Ref<EnvMapMaterial> envMapMaterial);
	void SetSkybox(Hazel::Ref<Hazel::HazelTextureCube> skybox);
	void Init();

	std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my); // EditorLayer::CastRay()
	std::pair<float, float> GetMouseViewportSpace();
	std::vector<glm::mat4> CalculateLightTransform(glm::mat4 lightProj, glm::vec3 position);

	void RenderSkybox();
	void RenderHazelGrid();
	void RenderOutline(Hazel::Ref<Shader> shader, Hazel::Entity entity, const glm::mat4& entityTransform, Hazel::Submesh& submesh);

public:
	Hazel::EditorCamera* m_EditorCamera;
	RuntimeCamera* m_RuntimeCamera;
	Hazel::HazelCamera* m_ActiveCamera;

	static SelectionMode s_SelectionMode;
	static Hazel::Ref<Hazel::HazelTexture2D> s_CheckerboardTexture;

	static Hazel::Ref<EnvMapMaterial> s_DefaultMaterial;
	static Hazel::Ref<EnvMapMaterial> s_LightMaterial;

	glm::mat4* m_CurrentlySelectedTransform = nullptr;
	glm::mat4* m_RelativeTransform = nullptr;
	bool m_AllowViewportCameraEvents = true; // EditorLayer (Raypicking)

	// viewports public
	bool m_IsViewportEnabled;
	glm::vec2 m_ImGuiViewportMain;
	glm::vec2 m_ViewportMainSize;
	Framebuffer* m_RenderFramebuffer;

private:
	Hazel::Ref<Shader> m_ShaderHazelPBR; // currently used PBR shader, m_ShaderHazelPBR_Anim or m_ShaderHazelPBR_Static
	Hazel::Ref<Shader> m_ShaderOutline;
	Hazel::Ref<Shader> m_ShaderShadow;
	Hazel::Ref<Shader> m_ShaderOmniShadow;

	CubeSkybox* m_SkyboxCube;
	Hazel::Ref<Hazel::HazelTextureCube> m_SkyboxTexture;
	float m_SkyboxExposureFactor = 1.0f;

	Quad* m_Quad;

	std::map<std::string, unsigned int>* m_SamplerSlots;

	// PBR params
	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	/** BEGIN properties Hazelnut/EditorLayer **/
	// Editor resources
	Hazel::Ref<Hazel::HazelTexture2D> m_PlayButtonTex;

	Hazel::Entity m_CameraEntity;

	Hazel::Entity m_DirectionalLightEntity;
	Hazel::Ref<ShadowMap> m_ShadowMapDirLight;
	glm::mat4 m_LightProjectionMatrix;
	glm::vec3 m_LightDirection; // temporary, use DirectionalLightComponent
	glm::mat4 m_DirLightTransform; // sent to shaders as an uniform dirLightTransform / u_DirLightTransform

	Hazel::Entity m_PointLightEntity; // temporary, for experimental use
	Hazel::Ref<OmniShadowMap> m_OmniShadowMapPointLight;

	Hazel::Entity m_SpotLightEntity;  // temporary, for experimental use
	Hazel::Ref<OmniShadowMap> m_OmniShadowMapSpotLight;

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

	glm::vec3 m_NewRay;
	glm::vec3 m_NewDir;

	glm::vec2 m_WorkPosImGui; // window offset on monitor real estate

	enum class SceneState
	{
		Edit = 0,
		Play,
		Pause,
	};
	SceneState m_SceneState;

	bool m_DisplayHazelGrid;
	bool m_DisplayBoundingBoxes;
	bool m_DisplayRay;
	bool m_DrawOnTopBoundingBoxes; // obsolete?
	bool m_DisplayLineElements;
	bool m_DisplayOutline;

	// Hazel LIVE! #014
	Hazel::Ref<Hazel::HazelScene> m_RuntimeScene;
	Hazel::Ref<Hazel::HazelScene> m_EditorScene;
	std::string m_SceneFilePath;
	bool m_ReloadScriptOnPlay = true;

	Hazel::SceneHierarchyPanel* m_SceneHierarchyPanel;

	// Hazel LIVE! #015
	bool m_UIShowBoundingBoxes;
	bool m_UIShowBoundingBoxesOnTop;

	std::string m_WindowTitleStatic = "";
	std::string m_WindowTitleDynamic = "";

	std::string m_StatusBarMessage = "";

};
