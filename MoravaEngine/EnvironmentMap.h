#pragma once

#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/HazelScene.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "Shader.h"
#include "TextureCubemap.h"
#include "Material.h"
#include "HazelFullscreenQuad.h"
#include "EnvMapMaterial.h"
#include "Scene.h"

#include <string>


class EnvironmentMap
{
	struct Hazel::HazelLight;
	struct Hazel::Environment;
	enum class Hazel::PrimitiveType;
	struct Data;
	struct Options;

public:
	EnvironmentMap() = default;
	EnvironmentMap(const std::string& filepath, Scene* scene);
	~EnvironmentMap();

	Hazel::Environment Load(const std::string& filepath);
	void SetEnvironment(Hazel::Environment environment);
	void Update(Scene* scene, float timestep);
	void AddEntity(Hazel::Entity* entity);
	Hazel::Entity* CreateEntity(const std::string& name);
	void LoadMesh(std::string fullPath);
	void LoadEnvMapMaterials(Mesh* mesh);
	void Render();
	void OnImGuiRender();

	void RenderHazelSkybox();
	void RenderHazelGrid();
	void CompositePassTemporary(Framebuffer* framebuffer);

	// Setters
	void SetSkyboxLOD(float LOD);

	// Getters
	inline Data* GetContextData() { return &m_Data; }
	inline Shader* GetShaderPBR_Anim() { return m_ShaderHazelPBR_Anim; }
	inline Shader* GetShaderPBR_Static() { return m_ShaderHazelPBR_Static; }
	inline Shader* GetShaderSkybox() { return m_ShaderSkybox; }
	inline Hazel::HazelTexture2D* GetEnvEquirect() { return m_EnvEquirect; }
	inline std::map<std::string, unsigned int>* GetSamplerSlots() { return m_SamplerSlots; }
	inline bool& GetRadiancePrefilter() { return m_RadiancePrefilter; }
	inline Hazel::HazelLight& GetLight() { return m_Data.SceneData.ActiveLight; }
	inline float& GetEnvMapRotation() { return m_EnvMapRotation; }
	inline Hazel::HazelTexture2D* GetCheckerboardTexture() { return m_CheckerboardTexture; }
	inline Hazel::HazelTextureCube* GetSkyboxTexture() { return m_SkyboxTexture; }
	inline Hazel::Entity* GetMeshEntity() { return m_MeshEntity; }
	inline void SetMeshEntity(Hazel::Entity* entity) { m_MeshEntity = entity; }
	inline float& GetSkyboxExposureFactor() { return m_SkyboxExposureFactor; };
	float& GetSkyboxLOD();
	Hazel::RenderPass* GetFinalRenderPass();
	FramebufferTexture* GetFinalColorBuffer();
	uint32_t GetFinalColorBufferID();

private:
	void SetupContextData();
	void SetupShaders();
	void UpdateUniforms();
	void UpdateShaderPBRUniforms(Shader* shaderHazelPBR, EnvMapMaterial* m_EnvMapMaterial);
	std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*> CreateEnvironmentMap(const std::string& filepath);
	void SetSkybox(Hazel::HazelTextureCube* skybox);

	// SceneRenderer
	void Init();
	void SetViewportSize(uint32_t width, uint32_t height);
	void BeginScene(const Scene* scene);
	void EndScene();
	void SubmitEntity(Hazel::Entity* entity);
	Options& GetOptions();
	void FlushDrawList();
	void GeometryPass();
	void CompositePass();

	// Renderer
	void BeginRenderPass(Hazel::RenderPass* renderPass, bool clear);
	void SubmitFullscreenQuad(Material* material);
	void DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest);
	void EndRenderPass();
	void SubmitMesh(Hazel::MeshAnimPBR* mesh, const glm::mat4& transform, Material* overrideMaterial);

private:
	struct Options
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	struct Data
	{
		const Hazel::HazelScene* ActiveScene = nullptr;
		struct SceneInfo
		{
			Camera* SceneCamera;

			// Resources
			Material* SkyboxMaterial;
			Hazel::Environment SceneEnvironment;
			Hazel::HazelLight ActiveLight;
		} SceneData;

		Hazel::HazelTexture2D* BRDFLUT;

		Hazel::RenderPass* GeoPass;
		Hazel::RenderPass* CompositePass;
		Hazel::RenderPass* ActiveRenderPass;

		struct DrawCommand
		{
			std::string Name;
			Mesh* Mesh;
			Material* Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;

		// Grid
		Material* GridMaterial;

		Options Options;

		// Renderer data
		Hazel::RenderCommandQueue* m_CommandQueue;

		unsigned int FullscreenQuadVAO;
		unsigned int FullscreenQuadVBO;
		unsigned int FullscreenQuadIBO;

	};
	Data m_Data;

	// Intermediate textures
	Hazel::HazelTextureCube* m_EnvUnfiltered;
	Hazel::HazelTexture2D* m_EnvEquirect;
	Hazel::HazelTextureCube* m_EnvFiltered;
	Hazel::HazelTextureCube* m_IrradianceMap;

	Shader* m_ShaderEquirectangularConversion;
	Shader* m_ShaderEnvFiltering;
	Shader* m_ShaderEnvIrradiance;
	Shader* m_ShaderSkybox;
	Shader* m_ShaderHazelPBR_Anim;
	Shader* m_ShaderHazelPBR_Static;
	Shader* m_ShaderComposite;
	Shader* m_ShaderGrid;
	Shader* m_ShaderHazelPBR; // currently used PBR shader, m_ShaderHazelPBR_Anim or m_ShaderHazelPBR_Static

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

	std::map<std::string, TextureInfo> m_TextureInfo;
	std::map<std::string, EnvMapMaterial*> m_EnvMapMaterials;

	HazelFullscreenQuad* m_HazelFullscreenQuad;

	float m_SkyboxExposureFactor = 2.0f;

};
