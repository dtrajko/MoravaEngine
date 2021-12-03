#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Editor/EditorCameraH2M.h"
#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/RenderCommandQueue.h"
#include "H2M/Renderer/RenderPass.h"
#include "H2M/Renderer/SceneEnvironmentH2M.h"

#include "H2M/Renderer/SceneRendererH2M.h"

#include "Camera/Camera.h"
#include "EnvMap/EnvMapSharedData.h"
#include "Mesh/HazelFullscreenQuad.h"


struct SceneRendererOptions
{
	bool ShowGrid = true;
	bool ShowSelectedInWireframe = false;

	bool ShowBoundingBoxes = false;

	enum class PhysicsColliderView
	{
		None = 0, Normal = 1, OnTop = 2
	};
	PhysicsColliderView ShowPhysicsColliders = PhysicsColliderView::None;

	//HBAO
	bool EnableHBAO = true;
	float HBAOIntensity = 1.5f;
	float HBAORadius = 1.0f;
	float HBAOBias = 0.35f;
	float HBAOBlurSharpness = 1.0f;
};

struct BloomSettings
{
	bool Enabled = true;
	float Threshold = 1.0f;
	float Knee = 0.1f;
	float UpsampleScale = 1.0f;
	float Intensity = 1.0f;
	float DirtIntensity = 1.0f;
};


class EnvMapSceneRenderer
{
public:
	static void Init(std::string filepath, H2M::SceneH2M* scene);

	static void SetViewportSize(uint32_t width, uint32_t height);

	static void BeginScene(H2M::SceneH2M* scene, const H2M::SceneRendererCameraH2M& camera);
	static void EndScene();

	static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<H2M::HazelMaterial> overrideMaterial = nullptr);
	static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

	static std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath);

	static H2M::RefH2M<H2M::RenderPass> GetFinalRenderPass();
	static FramebufferTexture* GetFinalColorBuffer(); // originally returns H2M::RefH2M<H2M::Texture2D_H2M>

	// TODO: Temp
	static uint32_t GetFinalColorBufferRendererID();

	static H2M::SceneRendererOptionsH2M& GetOptions();

	// Temporary methods from EnvMapEditorLayer
	static H2M::RefH2M<H2M::TextureCubeH2M> GetRadianceMap();
	static H2M::RefH2M<H2M::TextureCubeH2M> GetIrradianceMap();
	static H2M::RefH2M<H2M::Texture2D_H2M> GetBRDFLUT();
	static H2M::RefH2M<MoravaShader> GetShaderComposite();
	static H2M::RefH2M<H2M::RenderPass> GetGeoPass();
	static H2M::RefH2M<H2M::RenderPass> GetCompositePass();
	static void CreateDrawCommand(std::string fileNameNoExt, H2M::RefH2M<H2M::MeshH2M> mesh);
	static H2M::HazelDirLight& GetActiveLight();
	static void SetActiveLight(H2M::HazelDirLight& light);
	static void AddToDrawList(std::string name, H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, glm::mat4 transform);
	static H2M::Environment Load(const std::string& filepath);
	static void SetEnvironment(H2M::Environment environment);
	static H2M::RefH2M<MoravaShader> GetShaderSkybox() { return s_ShaderSkybox; }
	static H2M::RefH2M<MoravaShader> GetShaderGrid() { return s_ShaderGrid; }
	static H2M::RefH2M<H2M::Texture2D_H2M> GetEnvEquirect() { return s_EnvEquirect; }
	static void SetupShaders();
	static H2M::SceneRendererCameraH2M& GetCamera();

	static void SubmitEntity(H2M::EntityH2M entity);

	static void SubmitEntityEnvMap(H2M::EntityH2M entity);
	static glm::mat4 GetViewProjection();

	static void OnImGuiRender();

	// Moved from EnvMapEditorLayer back to EnvMapSceneRenderer
	static void FlushDrawList();
	static void GeometryPass();
	static void CompositePass();
	static void BloomBlurPass();
	static void ShadowMapPass();

private:
	static void RenderSkybox();
	static void RenderHazelGrid();
	static void RenderOutline(H2M::RefH2M<MoravaShader> shader, H2M::EntityH2M entity, const glm::mat4& entityTransform, H2M::RefH2M<H2M::SubmeshH2M> submesh);
	static void UpdateShaderPBRUniforms(H2M::RefH2M<MoravaShader> shaderHazelPBR, H2M::RefH2M<EnvMapMaterial> envMapMaterial);

public:
	// From EnvironmentMap
	static H2M::RefH2M<MoravaShader> s_ShaderEquirectangularConversion;
	static H2M::RefH2M<MoravaShader> s_ShaderEnvFiltering;
	static H2M::RefH2M<MoravaShader> s_ShaderEnvIrradiance;
	static H2M::RefH2M<MoravaShader> s_ShaderGrid;
	static H2M::RefH2M<MoravaShader> s_ShaderSkybox;

	// Intermediate textures
	// static H2M::RefH2M<H2M::TextureCubeH2M> s_EnvUnfiltered;
	static H2M::RefH2M<H2M::Texture2D_H2M> s_EnvEquirect;
	// static H2M::RefH2M<H2M::TextureCubeH2M> s_EnvFiltered;
	// static H2M::RefH2M<H2M::TextureCubeH2M> s_IrradianceMap;

	static float s_GridScale;
	static float s_GridSize;

	static uint32_t s_FramebufferWidth;
	static uint32_t s_FramebufferHeight;

	// From SceneRenderer
	static H2M::RefH2M<H2M::RenderCommandBuffer> s_CommandBuffer;

	struct UBRendererData
	{
		glm::vec4 CascadeSplits;
		uint32_t TilesCountX{ 0 };
		bool ShowCascades = false;
		char Padding0[3] = { 0, 0, 0 }; // Bools are 4-bytes in GLSL
		bool SoftShadows = true;
		char Padding1[3] = { 0, 0, 0 };
		float LightSize = 0.5f;
		float MaxShadowDistance = 200.0f;
		float ShadowFade = 1.0f;
		bool CascadeFading = true;
		char Padding2[3] = { 0, 0, 0 };
		float CascadeTransitionFade = 1.0f;
		bool ShowLightComplexity = false;
		char Padding3[3] = { 0,0,0 };
	};
	static UBRendererData s_RendererDataUB;

	static float s_CascadeSplitLambda;
	static float s_CascadeFarPlaneOffset;
	static float s_CascadeNearPlaneOffset;

	static H2M::RefH2M<H2M::Pipeline> s_GeometryPipeline;
	static H2M::RefH2M<H2M::Pipeline> s_SelectedGeometryPipeline;
	static H2M::RefH2M<H2M::Pipeline> s_GeometryWireframePipeline;
	static H2M::RefH2M<H2M::Pipeline> s_GeometryWireframeOnTopPipeline;
	static H2M::RefH2M<H2M::Pipeline> s_PreDepthPipeline;
	static H2M::RefH2M<H2M::Pipeline> s_CompositePipeline;
	static H2M::RefH2M<H2M::Pipeline> s_ShadowPassPipelines[4];
	static H2M::RefH2M<H2M::HazelMaterial> s_ShadowPassMaterial;
	static H2M::RefH2M<H2M::HazelMaterial> s_PreDepthMaterial;
	static H2M::RefH2M<H2M::Pipeline> s_SkyboxPipeline;
	static H2M::RefH2M<H2M::HazelMaterial> s_SkyboxMaterial;
	static H2M::RefH2M<H2M::Pipeline> s_DOFPipeline;
	static H2M::RefH2M<H2M::HazelMaterial> s_DOFMaterial;

	static SceneRendererOptions s_Options;

	// Bloom compute
	static H2M::RefH2M<H2M::Texture2D_H2M> s_BloomComputeTextures[3];

	static bool s_ResourcesCreated;

	static BloomSettings s_BloomSettings;
	static H2M::RefH2M<H2M::Texture2D_H2M> s_BloomDirtTexture;

	struct GPUTimeQueries
	{
		uint32_t ShadowMapPassQuery = 0;
		uint32_t DepthPrePassQuery = 0;
		uint32_t LightCullingPassQuery = 0;
		uint32_t GeometryPassQuery = 0;
		uint32_t HBAOPassQuery = 0;
		uint32_t BloomComputePassQuery = 0;
		uint32_t JumpFloodPassQuery = 0;
		uint32_t CompositePassQuery = 0;
	};
	
	static GPUTimeQueries s_GPUTimeQueries;

	static H2M::RefH2M<H2M::Renderer2D> s_Renderer2D;

};
