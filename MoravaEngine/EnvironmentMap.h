#pragma once

#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/HazelScene.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "Shader.h"
#include "TextureCubemap.h"
#include "Material.h"
#include "EnvMapMaterial.h"
#include "Scene.h"

#include <string>


class EnvironmentMap
{
	struct Hazel::HazelLight;
	struct Hazel::Environment;
	enum class Hazel::PrimitiveType;

public:
	EnvironmentMap() = default;
	EnvironmentMap(const std::string& filepath, Scene* scene);
	~EnvironmentMap();

	void Update(Scene* scene, float timestep);
	Hazel::Entity* CreateEntity(const std::string& name);
	void LoadMesh(std::string fullPath);
	void LoadEnvMapMaterials(Mesh* mesh);
	void Render(Framebuffer* framebuffer);
	void OnImGuiRender();

	void RenderHazelSkybox();
	void RenderHazelGrid();

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
	inline Hazel::Entity* GetMeshEntity() { return m_MeshEntity; }
	inline void SetMeshEntity(Hazel::Entity* entity) { m_MeshEntity = entity; }
	inline float& GetSkyboxExposureFactor() { return m_SkyboxExposureFactor; };
	float& GetSkyboxLOD();
	void SetViewportBounds(glm::vec2* viewportBounds);
	inline Hazel::SceneRenderer* GetSceneRenderer() { return m_SceneRenderer; }
	inline bool* GetDisplayHazelGrid() { return &m_DisplayHazelGrid; }

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
	void SubmitEntity(Hazel::Entity* entity);

	// Renderer
	void DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest);
	void SubmitMesh(Hazel::MeshAnimPBR* mesh, const glm::mat4& transform, Material* overrideMaterial);

	// Renderer2D::BeginScene
	void Renderer2D_Init();
	void Renderer2D_BeginScene(const glm::mat4& viewProj, bool depthTest);

	// Raypicking (EditorLayer)
public:
	void OnEvent(Event& e); // EditorLayer::OnEvent()

private:
	bool OnKeyPressedEvent(KeyPressedEvent& e); // EditorLayer::OnKeyPressedEvent()
	bool OnMouseButtonPressed(MouseButtonPressedEvent& e); // EditorLayer::OnMouseButtonPressedEvent()
	std::pair<float, float> GetMouseViewportSpace();
	std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my); // EditorLayer::CastRay()
	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color); // Renderer2D::DrawLine()
	void Renderer2D_FlushAndResetLines(); // Renderer2D::FlushAndResetLines()
	void DrawAABB(const Hazel::AABB& aabb, const glm::mat4& transform, glm::vec4& color = glm::vec4(1.0f)); // Renderer::DrawAABB()
	void DrawAABB(Mesh* mesh, const glm::mat4& transform, glm::vec4& color = glm::vec4(1.0f)); // Renderer::DrawAABB()

private:
	// TODO:  Move to Renderer2D 
	struct Renderer2DData
	{
		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		uint32_t TextureSlotIndex = 1; // 0 = white texture

		// Lines
		Hazel::VertexArray* LineVertexArray;
		Hazel::VertexBuffer* LineVertexBuffer;

		uint32_t LineIndexCount = 0;
		Hazel::LineVertex* LineVertexBufferBase = nullptr;
		Hazel::LineVertex* LineVertexBufferPtr = nullptr;

		glm::mat4 CameraViewProj;
		bool DepthTest = true;

		Hazel::Renderer2D::Statistics Stats;
	};
	Renderer2DData m_R2DData;

	Shader* m_ShaderHazelPBR_Anim;
	Shader* m_ShaderHazelPBR_Static;
	Shader* m_ShaderHazelPBR; // currently used PBR shader, m_ShaderHazelPBR_Anim or m_ShaderHazelPBR_Static
	Shader* m_ShaderRenderer2D; // Renderer2D::s_Data.TextureShader
	Shader* m_ShaderRenderer2D_Line; // Renderer2D::s_Data.LineShader

	Hazel::HazelTextureCube* m_SkyboxTexture;

	std::map<std::string, unsigned int>* m_SamplerSlots;

	// PBR params
	bool m_RadiancePrefilter = false;

	float m_EnvMapRotation = 0.0f;

	/** BEGIN properties EditorLayer **/
	// Editor resources
	Hazel::HazelTexture2D* m_CheckerboardTexture;

	glm::vec2 m_ViewportBounds[2];
	int m_GizmoType = -1; // -1 = no gizmo
	Hazel::Entity* m_MeshEntity = nullptr;
	/** END properties EditorLayer **/

	// Materials
	float m_MaterialSpecular = 0.0f;
	float m_MaterialShininess = 0.0f;

	TextureInfo m_TextureInfoDefault;
	std::map<std::string, TextureInfo> m_TextureInfo;
	std::map<std::string, EnvMapMaterial*> m_EnvMapMaterials;

	float m_SkyboxExposureFactor = 2.0f;

	// Raypicking (EditorLayer)
	bool m_AllowViewportCameraEvents = true;
	bool m_DrawOnTopBoundingBoxes = true;
	glm::vec3 m_NewRay;
	std::vector<Hazel::Submesh> m_SelectedSubmeshes;

	Hazel::SceneRenderer* m_SceneRenderer;

	bool m_DisplayHazelGrid;

};
