#pragma once

#include "Shader.h"
#include "TextureCubemap.h"
#include "Material.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/VertexArray.h"
#include "CubeSkybox.h" // Skybox temporary version
#include "HazelFullscreenQuad.h"

#include <string>


class EnvironmentMap
{
	struct Data;
	struct LightStruct;
	struct AlbedoInput;
	struct NormalInput;
	struct MetalnessInput;
	struct RoughnessInput;
	struct Options;
	struct Environment;
	enum class PrimitiveType;

public:
	EnvironmentMap() = default;
	EnvironmentMap(const std::string& filepath);
	~EnvironmentMap();

	Environment Load(const std::string& filepath);
	void SetEnvironment(Environment environment);
	void Update(Scene* scene, float timestep);
	void AddEntity(Hazel::Entity* entity);
	Hazel::Entity* CreateEntity(const std::string& name);

	void Render();

	void RenderHazelSkybox();
	void RenderHazelGrid();

	// Setters
	inline void SetSkyboxLOD(float LOD) { m_SkyboxLOD = LOD; }

	// Getters
	inline Data* GetContextData() { return &m_Data; }
	inline Shader* GetPBRShader() { return m_ShaderHazelAnimPBR; }
	inline Shader* GetShaderSkybox() { return m_ShaderSkybox; }
	inline Hazel::HazelTexture2D* GetEnvEquirect() { return m_EnvEquirect; }
	inline std::map<std::string, unsigned int>* GetSamplerSlots() { return m_SamplerSlots; }
	inline bool& GetRadiancePrefilter() { return m_RadiancePrefilter; }
	inline float& GetSkyboxLOD() { return m_SkyboxLOD; }
	inline LightStruct& GetLight() { return m_Data.SceneData.ActiveLight; }
	inline float& GetEnvMapRotation() { return m_EnvMapRotation; }
	inline Hazel::HazelTexture2D* GetCheckerboardTexture() { return m_CheckerboardTexture; }
	inline AlbedoInput& GetAlbedoInput() { return m_AlbedoInput; }
	inline NormalInput& GetNormalInput() { return m_NormalInput; }
	inline MetalnessInput& GetMetalnessInput() { return m_MetalnessInput; }
	inline RoughnessInput& GetRoughnessInput() { return m_RoughnessInput; }
	inline Hazel::HazelTextureCube* GetSkyboxTexture() { return m_SkyboxTexture; }
	inline Hazel::Entity* GetMeshEntity() { return m_MeshEntity; }
	inline void SetMeshEntity(Hazel::Entity* entity) { m_MeshEntity = entity; }
	Hazel::RenderPass* GetFinalRenderPass();
	FramebufferTexture* GetFinalColorBuffer();
	uint32_t GetFinalColorBufferID();

private:
	void SetupContextData();
	// void SetupFullscreenQuad();
	void SetupShaders();
	void UpdateUniforms();
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
	void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest);
	void EndRenderPass();
	void SubmitMesh(Hazel::MeshAnimPBR* mesh, const glm::mat4& transform, Material* overrideMaterial);


private:
	struct Environment
	{
		Hazel::HazelTextureCube* RadianceMap;
		Hazel::HazelTextureCube* IrradianceMap;
	};

	struct LightStruct
	{
		glm::vec3 Direction;
		glm::vec3 Radiance;

		float Multiplier = 1.0f;
	};

	struct Options
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	enum class PrimitiveType
	{
		None = 0, Triangles, Lines
	};

	struct Data
	{
		const Scene* ActiveScene = nullptr;
		struct SceneInfo
		{
			Camera* SceneCamera;

			// Resources
			Material* SkyboxMaterial;
			Environment SceneEnvironment;
			LightStruct ActiveLight;
		} SceneData;

		Hazel::HazelTexture2D* BRDFLUT;

		Hazel::RenderPass* GeoPass;
		Hazel::RenderPass* CompositePass;
		Hazel::RenderPass* ActiveRenderPass;

		struct DrawCommand
		{
			std::string Name;
			Hazel::MeshAnimPBR* Mesh;
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
	Shader* m_ShaderHazelAnimPBR;
	Shader* m_ShaderComposite;
	Shader* m_ShaderGrid;

	Hazel::HazelTextureCube* m_SkyboxTexture;

	std::map<std::string, unsigned int>* m_SamplerSlots;

	/**** BEGIN properties Scene ****/
	float m_SkyboxLOD = 1.0f;
	std::vector<Hazel::Entity*> m_Entities;
	/**** END properties Scene ****/

	/**** BEGIN properties EditorLayer ****/
	struct AlbedoInput
	{
		glm::vec3 Color = { 0.972f, 0.96f, 0.915f }; // Silver, from https://docs.unrealengine.com/en-us/Engine/Rendering/Materials/PhysicallyBased
		Hazel::HazelTexture2D* TextureMap;
		bool SRGB = true;
		bool UseTexture = false;
	};
	AlbedoInput m_AlbedoInput;

	struct NormalInput
	{
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	NormalInput m_NormalInput;

	struct MetalnessInput
	{
		float Value = 1.0f;
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	MetalnessInput m_MetalnessInput;

	struct RoughnessInput
	{
		float Value = 0.2f;
		Hazel::HazelTexture2D* TextureMap;
		bool UseTexture = false;
	};
	RoughnessInput m_RoughnessInput;

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

	float m_MaterialSpecular = 0.0f;
	float m_MaterialShininess = 0.0f;

	// Skybox temporary version
	CubeSkybox* m_SkyboxCube;

	HazelFullscreenQuad* m_HazelFullscreenQuad;

};
