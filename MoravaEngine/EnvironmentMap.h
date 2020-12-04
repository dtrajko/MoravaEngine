#pragma once

#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/HazelScene.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Core/Math/Ray.h"

#include "Shader.h"
#include "TextureCubemap.h"
#include "Material.h"
#include "EnvMapMaterial.h"
#include "Scene.h"
#include "Quad.h"
#include "CubeSkybox.h"

#include <string>


class EnvironmentMap
{
	struct SelectedSubmesh;

public:
	EnvironmentMap() = default;
	EnvironmentMap(const std::string& filepath, Scene* scene);
	~EnvironmentMap();

	void Update(Scene* scene, float timestep);
	void UpdateImGuizmo(Window* mainWindow);
	Hazel::Entity CreateEntity(const std::string& name);
	Hazel::Entity LoadEntity(std::string fullPath);
	void LoadEnvMapMaterials(Mesh* mesh);
	void Render(Framebuffer* framebuffer);
	void OnImGuiRender();

	// Setters
	void SetSkyboxLOD(float LOD);

	// Getters
	inline Shader* GetShaderPBR_Anim() { return m_ShaderHazelPBR_Anim; }
	inline Shader* GetShaderPBR_Static() { return m_ShaderHazelPBR_Static; }
	inline std::map<std::string, unsigned int>* GetSamplerSlots() { return m_SamplerSlots; }
	inline bool& GetRadiancePrefilter() { return m_RadiancePrefilter; }
	inline float& GetEnvMapRotation() { return m_EnvMapRotation; }
	inline Hazel::HazelTexture2D* GetCheckerboardTexture() { return m_CheckerboardTexture; }
	inline Hazel::HazelTextureCube* GetSkyboxTexture() { return m_SkyboxTexture; }
	Hazel::Entity* GetMeshEntity();
	inline float& GetSkyboxExposureFactor() { return m_SkyboxExposureFactor; };
	float& GetSkyboxLOD();
	void SetViewportBounds(glm::vec2* viewportBounds);
	inline Hazel::SceneRenderer* GetSceneRenderer() { return m_SceneRenderer; }
	inline bool* GetDisplayHazelGrid() { return &m_DisplayHazelGrid; }
	inline bool* GetDisplayBoundingBoxes() { return &m_DisplayBoundingBoxes; };

private:
	void SetupContextData();
	void SetupShaders();
	void UpdateUniforms();
	void UpdateShaderPBRUniforms(Shader* shaderHazelPBR, EnvMapMaterial* m_EnvMapMaterial);
	void SetSkybox(Hazel::HazelTextureCube* skybox);
	void Init();

	// SceneRenderer
public:
	void CompositePassTemporary(Framebuffer* framebuffer);
	void GeometryPassTemporary();
	void SubmitEntity(Hazel::Entity entity);

	// Renderer
	void DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest);
	void SubmitMesh(Hazel::HazelMesh* mesh, const glm::mat4& transform, Material* overrideMaterial);

	// EditorLayer
	void OnEvent(Event& e);

private:
	bool OnKeyPressedEvent(KeyPressedEvent& e); // EditorLayer::OnKeyPressedEvent()
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e); // EditorLayer::OnMouseButtonPressedEvent()
	void OnSelected(const SelectedSubmesh& selectionContext);
	std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my); // EditorLayer::CastRay()
	std::pair<float, float> GetMouseViewportSpace();
	EnvMapMaterial* CreateDefaultMaterial(const std::string& nodeName);
	void RenderSkybox();
	void RenderHazelGrid();

public:
	glm::mat4* m_CurrentlySelectedTransform = nullptr;
	glm::mat4* m_RelativeTransform = nullptr;
	bool m_AllowViewportCameraEvents = true; // EditorLayer (Raypicking)

private:
	Shader* m_ShaderHazelPBR_Anim;
	Shader* m_ShaderHazelPBR_Static;
	Shader* m_ShaderHazelPBR; // currently used PBR shader, m_ShaderHazelPBR_Anim or m_ShaderHazelPBR_Static
	Shader* m_ShaderRenderer2D; // Renderer2D::s_Data.TextureShader
	Shader* m_ShaderRenderer2D_Line; // Renderer2D::s_Data.LineShader

	CubeSkybox* m_SkyboxCube;
	Hazel::HazelTextureCube* m_SkyboxTexture;
	float m_SkyboxExposureFactor = 2.0f;

	Quad* m_Quad;

	std::map<std::string, unsigned int>* m_SamplerSlots;

	// PBR params
	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	/** BEGIN properties Hazelnut/EditorLayer **/
	// Editor resources
	Hazel::HazelTexture2D* m_CheckerboardTexture;

	Hazel::Entity m_CameraEntity;

	int m_GizmoType = -1; // -1 = no gizmo
	float m_SnapValue = 0.5f;
	glm::vec2 m_ViewportBounds[2];
	float m_ViewportWidth = 0.0f;
	float m_ViewportHeight = 0.0f;
	/** END properties Hazelnut/EditorLayer **/

	// Materials
	float m_MaterialSpecular = 0.0f;
	float m_MaterialShininess = 0.0f;

	TextureInfo m_TextureInfoDefault;
	std::map<std::string, TextureInfo> m_TextureInfo;
	std::map<std::string, EnvMapMaterial*> m_EnvMapMaterials;

	bool m_DrawOnTopBoundingBoxes = true;
	glm::vec3 m_NewRay;
	glm::vec3 m_NewDir;

	struct SelectedSubmesh
	{
		Hazel::Entity Entity;
		Hazel::Submesh* Mesh;
		float Distance;
	};
	std::vector<SelectedSubmesh> m_SelectionContext;

	enum class SelectionMode
	{
		None = 0,
		Entity = 1,
		SubMesh = 2,
	};
	SelectionMode m_SelectionMode = SelectionMode::Entity;

	Hazel::SceneRenderer* m_SceneRenderer;

	bool m_DisplayHazelGrid;

	bool m_DisplayBoundingBoxes;

	glm::vec2 m_WorkPosImGui; // window offset on monitor real estate

};
