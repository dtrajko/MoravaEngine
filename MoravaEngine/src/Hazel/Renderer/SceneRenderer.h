#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "RenderCommandBuffer.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/HazelScene.h"
#include "HazelMesh.h"
#include "RenderPass.h"


namespace Hazel {

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

	struct SceneRendererCamera
	{
		Hazel::HazelCamera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
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

	class SceneRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const HazelScene* scene, const SceneRendererCamera& camera);
		static void EndScene();

		// old?
		static void SubmitMesh(MeshComponent meshComponent, TransformComponent transformComponent);
		static void SubmitSelectedMesh(MeshComponent meshComponent, TransformComponent transformComponent);

		// new?
		static void SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<HazelMaterial> overrideMaterial = Ref<HazelMaterial>());
		static void SubmitSelectedMesh(Ref<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<HazelTexture2D> GetFinalColorBuffer();

		// TODO: Temp
		static uint32_t GetFinalColorBufferRendererID();

		static SceneRendererOptions& GetOptions();

		void OnImGuiRender();

	private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
		static void ShadowMapPass();

	private:
		Ref<RenderCommandBuffer> m_CommandBuffer;

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

		Ref<Pipeline> m_GeometryPipeline;
		Ref<Pipeline> m_SelectedGeometryPipeline;
		Ref<Pipeline> m_GeometryWireframePipeline;
		Ref<Pipeline> m_GeometryWireframeOnTopPipeline;
		Ref<Pipeline> m_PreDepthPipeline;
		Ref<Pipeline> m_CompositePipeline;
		Ref<Pipeline> m_ShadowPassPipelines[4];
		Ref<Material> m_ShadowPassMaterial;
		Ref<Material> m_PreDepthMaterial;
		Ref<Pipeline> m_SkyboxPipeline;
		Ref<Material> m_SkyboxMaterial;

		Ref<Pipeline> m_DOFPipeline;
		Ref<Material> m_DOFMaterial;

		SceneRendererOptions m_Options;

		// Jump Flood Pass

		// Bloom compute
		Ref<HazelTexture2D> m_BloomComputeTextures[3];

		bool m_ResourcesCreated = false;

		BloomSettings m_BloomSettings;
		Ref<HazelTexture2D> m_BloomDirtTexture;

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
