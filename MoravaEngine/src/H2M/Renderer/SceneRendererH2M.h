/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "RenderCommandBufferH2M.h"
#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Scene/SceneH2M.h"
#include "MeshH2M.h"
#include "RenderPassH2M.h"


namespace H2M
{

	struct SceneRendererOptionsH2M
	{
		bool ShowGrid = true;
		bool ShowSelectedInWireframe = false;

		bool ShowBoundingBoxes = false;

		enum class PhysicsColliderViewH2M
		{
			None = 0, Normal = 1, OnTop = 2
		};
		PhysicsColliderViewH2M ShowPhysicsColliders = PhysicsColliderViewH2M::None;

		//HBAO
		bool EnableHBAO = true;
		float HBAOIntensity = 1.5f;
		float HBAORadius = 1.0f;
		float HBAOBias = 0.35f;
		float HBAOBlurSharpness = 1.0f;
	};

	struct SceneRendererCameraH2M
	{
		CameraH2M Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	struct BloomSettingsH2M
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.1f;
		float UpsampleScale = 1.0f;
		float Intensity = 1.0f;
		float DirtIntensity = 1.0f;
	};

	struct SceneRendererSpecificationH2M
	{
		bool SwapChainTarget = false;
	};

	class SceneRendererH2M : public RefCountedH2M
	{
	public:
		SceneRendererH2M(RefH2M<SceneH2M> scene, SceneRendererSpecificationH2M specification = SceneRendererSpecificationH2M{});

		static void Init();
		static void Shutdown();

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const SceneH2M* scene, const SceneRendererCameraH2M& camera);
		static void EndScene();

		// old?
		static void SubmitMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent);
		static void SubmitSelectedMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent);

		// new?
		static void SubmitMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f), RefH2M<MaterialH2M> overrideMaterial = RefH2M<MaterialH2M>());
		static void SubmitSelectedMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath);

		static RefH2M<RenderPassH2M> GetFinalRenderPass();
		static RefH2M<Texture2D_H2M> GetFinalColorBuffer();

		// TODO: Temp
		static uint32_t GetFinalColorBufferRendererID();

		static SceneRendererOptionsH2M& GetOptions();

		void OnImGuiRender();

	private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
		static void ShadowMapPass();

	private:
		RefH2M<RenderCommandBufferH2M> m_CommandBuffer;

		struct UBRendererData
		{
			glm::vec4 CascadeSplits;
			uint32_t TilesCountX{ 0 };
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 }; // Bools are 4-bytes in GLSL
			bool SoftShadows = true;
			char Padding1[3] = { 0,0,0 };
			float LightSize = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade = 1.0f;
			bool CascadeFading = true;
			char Padding2[3] = { 0,0,0 };
			float CascadeTransitionFade = 1.0f;
			bool ShowLightComplexity = false;
			char Padding3[3] = { 0,0,0 };
		} RendererDataUB;

		float CascadeSplitLambda = 0.92f;
		float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;

		RefH2M<PipelineH2M> m_GeometryPipeline;
		RefH2M<PipelineH2M> m_SelectedGeometryPipeline;
		RefH2M<PipelineH2M> m_GeometryWireframePipeline;
		RefH2M<PipelineH2M> m_GeometryWireframeOnTopPipeline;
		RefH2M<PipelineH2M> m_PreDepthPipeline;
		RefH2M<PipelineH2M> m_CompositePipeline;
		RefH2M<PipelineH2M> m_ShadowPassPipelines[4];
		RefH2M<Material> m_ShadowPassMaterial;
		RefH2M<Material> m_PreDepthMaterial;
		RefH2M<PipelineH2M> m_SkyboxPipeline;
		RefH2M<Material> m_SkyboxMaterial;

		RefH2M<PipelineH2M> m_DOFPipeline;
		RefH2M<Material> m_DOFMaterial;

		SceneRendererOptionsH2M m_Options;

		// Jump Flood Pass

		// Bloom compute
		RefH2M<Texture2D_H2M> m_BloomComputeTextures[3];

		bool m_ResourcesCreated = false;

		BloomSettingsH2M m_BloomSettings;
		RefH2M<Texture2D_H2M> m_BloomDirtTexture;

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
		} m_GPUTimeQueries;
	};

}
