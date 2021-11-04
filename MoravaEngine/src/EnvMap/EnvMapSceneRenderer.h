#define _CRT_SECURE_NO_WARNINGS

#pragma once


#include "Hazel/Core/Ref.h"
#include "Hazel/Editor/EditorCamera.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/SceneEnvironment.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "HazelLegacy/Scene/EntityHazelLegacy.h"

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
	static void Init(std::string filepath, Hazel::SceneHazelLegacy* scene);

	static void SetViewportSize(uint32_t width, uint32_t height);

	static void BeginScene(Hazel::SceneHazelLegacy* scene, const Hazel::SceneRendererCamera& camera);
	static void EndScene();

	static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Hazel::HazelMaterial> overrideMaterial = nullptr);
	static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

	static std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

	static Hazel::Ref<Hazel::RenderPass> GetFinalRenderPass();
	static FramebufferTexture* GetFinalColorBuffer(); // originally returns Hazel::Ref<Hazel::HazelTexture2D>

	// TODO: Temp
	static uint32_t GetFinalColorBufferRendererID();

	static Hazel::SceneRendererOptions& GetOptions();

	// Temporary methods from EnvMapEditorLayer
	static Hazel::Ref<Hazel::HazelTextureCube> GetRadianceMap();
	static Hazel::Ref<Hazel::HazelTextureCube> GetIrradianceMap();
	static Hazel::Ref<Hazel::HazelTexture2D> GetBRDFLUT();
	static Hazel::Ref<MoravaShader> GetShaderComposite();
	static Hazel::Ref<Hazel::RenderPass> GetGeoPass();
	static Hazel::Ref<Hazel::RenderPass> GetCompositePass();
	static void CreateDrawCommand(std::string fileNameNoExt, Hazel::Ref<Hazel::MeshHazelLegacy> mesh);
	static Hazel::HazelDirLight& GetActiveLight();
	static void SetActiveLight(Hazel::HazelDirLight& light);
	static void AddToDrawList(std::string name, Hazel::Ref<Hazel::MeshHazelLegacy> mesh, Hazel::EntityHazelLegacy entity, glm::mat4 transform);
	static Hazel::Environment Load(const std::string& filepath);
	static void SetEnvironment(Hazel::Environment environment);
	static Hazel::Ref<MoravaShader> GetShaderSkybox() { return s_ShaderSkybox; }
	static Hazel::Ref<MoravaShader> GetShaderGrid() { return s_ShaderGrid; }
	static Hazel::Ref<Hazel::HazelTexture2D> GetEnvEquirect() { return s_EnvEquirect; }
	static void SetupShaders();
	static Hazel::SceneRendererCamera& GetCamera();

	static void SubmitEntity(Hazel::EntityHazelLegacy entity);

	static void SubmitEntityEnvMap(Hazel::EntityHazelLegacy entity);
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
	static void RenderOutline(Hazel::Ref<MoravaShader> shader, Hazel::EntityHazelLegacy entity, const glm::mat4& entityTransform, Hazel::SubmeshHazelLegacy& submesh);
	static void UpdateShaderPBRUniforms(Hazel::Ref<MoravaShader> shaderHazelPBR, Hazel::Ref<EnvMapMaterial> envMapMaterial);

public:
	// From EnvironmentMap
	static Hazel::Ref<MoravaShader> s_ShaderEquirectangularConversion;
	static Hazel::Ref<MoravaShader> s_ShaderEnvFiltering;
	static Hazel::Ref<MoravaShader> s_ShaderEnvIrradiance;
	static Hazel::Ref<MoravaShader> s_ShaderGrid;
	static Hazel::Ref<MoravaShader> s_ShaderSkybox;

	// Intermediate textures
	// static Hazel::Ref<Hazel::HazelTextureCube> s_EnvUnfiltered;
	static Hazel::Ref<Hazel::HazelTexture2D> s_EnvEquirect;
	// static Hazel::Ref<Hazel::HazelTextureCube> s_EnvFiltered;
	// static Hazel::Ref<Hazel::HazelTextureCube> s_IrradianceMap;

	static float s_GridScale;
	static float s_GridSize;

	static uint32_t s_FramebufferWidth;
	static uint32_t s_FramebufferHeight;

	// From SceneRenderer
	static Hazel::Ref<Hazel::RenderCommandBuffer> s_CommandBuffer;

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

	static Hazel::Ref<Hazel::Pipeline> s_GeometryPipeline;
	static Hazel::Ref<Hazel::Pipeline> s_SelectedGeometryPipeline;
	static Hazel::Ref<Hazel::Pipeline> s_GeometryWireframePipeline;
	static Hazel::Ref<Hazel::Pipeline> s_GeometryWireframeOnTopPipeline;
	static Hazel::Ref<Hazel::Pipeline> s_PreDepthPipeline;
	static Hazel::Ref<Hazel::Pipeline> s_CompositePipeline;
	static Hazel::Ref<Hazel::Pipeline> s_ShadowPassPipelines[4];
	static Hazel::Ref<Hazel::HazelMaterial> s_ShadowPassMaterial;
	static Hazel::Ref<Hazel::HazelMaterial> s_PreDepthMaterial;
	static Hazel::Ref<Hazel::Pipeline> s_SkyboxPipeline;
	static Hazel::Ref<Hazel::HazelMaterial> s_SkyboxMaterial;
	static Hazel::Ref<Hazel::Pipeline> s_DOFPipeline;
	static Hazel::Ref<Hazel::HazelMaterial> s_DOFMaterial;

	static SceneRendererOptions s_Options;

	// Bloom compute
	static Hazel::Ref<Hazel::HazelTexture2D> s_BloomComputeTextures[3];

	static bool s_ResourcesCreated;

	static BloomSettings s_BloomSettings;
	static Hazel::Ref<Hazel::HazelTexture2D> s_BloomDirtTexture;

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

	static Hazel::Ref<Hazel::Renderer2D> s_Renderer2D;

};
