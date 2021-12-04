#pragma once

#include "H2M/Renderer/SceneRenderer.h"
#include "H2M/Scene/HazelScene.h"

#include "H2M/Scene/ComponentsH2M.h"


namespace H2M {

	struct SceneRendererOptionsVulkan
	{
		bool ShowGrid = true;
		bool ShowSelectedInWireframe = false;

		enum class PhysicsColliderView
		{
			None = 0,
			Normal = 1,
			OnTop = 2,
		};

		PhysicsColliderView ShowPhysicsColliders = PhysicsColliderView::None;
		glm::vec4 PhysicsColliderColor = glm::vec4{ 0.2f, 1.0f, 0.2f, 1.0f };

		// HBAO
		bool EnableHBAO = true;
		float HBAOIntensity = 1.5f;
		float HBAORadius = 1.0f;
		float HBAOBias = 0.35f;
		float HBAOBlurSharpness = 1.0f;

		bool ShowBoundingBoxes = false;
	};

	struct SceneRendererCameraVulkan
	{
		HazelCamera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	struct BloomSettingsVulkan
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.1f;
		float UpsampleScale = 1.0f;
		float Intensity = 1.0f;
		float DirtIntensity = 1.0f;
	};

	struct SceneRendererSpecificationVulkan
	{
		bool SwapChainTarget = false;
	};

	class SceneRendererVulkan : public RefCounted
	{
	public:
		SceneRendererVulkan(RefH2M<HazelScene> scene, SceneRendererSpecificationVulkan specification = SceneRendererSpecificationVulkan{});

		static void Init();
		static void Shutdown();

		void SetScene(RefH2M<HazelScene> scene);

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const HazelScene* scene, const SceneRendererCameraVulkan& camera);
		static void EndScene();
		void UpdateHBAOData();

		static void SubmitMesh(MeshComponentH2M meshComponent, TransformComponent transformComponent);
		static void SubmitSelectedMesh(MeshComponentH2M meshComponent, TransformComponent transformComponent);

		static void SubmitMesh(RefH2M<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), RefH2M<MaterialH2M> overrideMaterial = RefH2M<MaterialH2M>());
		static void SubmitSelectedMesh(RefH2M<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static RefH2M<RenderPass> GetFinalRenderPass();
		static RefH2M<Texture2D_H2M> GetFinalPassImage(); // previously: GetFinalColorBuffer

		static SceneRendererOptionsVulkan& GetOptions();

		void SetLineWidth(float width);

		static void WaitForThreads();

private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
		static void ShadowMapPass();

	private:
		RefH2M<HazelScene> m_Scene;
		SceneRendererSpecificationVulkan m_Specification;
		RefH2M<RenderCommandBuffer> m_CommandBuffer;

	private:
		bool m_NeedsResize = false;
		bool m_Active = false;
		bool m_ResourcesCreated = false;

		float m_LineWidth = 2.0f;

	};

}
