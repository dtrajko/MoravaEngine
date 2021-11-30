#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/HazelCamera.h"
#include "H2M/Core/Ref.h"

#include "H2M/Scene/ComponentsH2M.h"

#include <glm/glm.hpp>


namespace H2M {

	struct SceneRendererOptionsH2M
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

	struct SceneRendererCameraH2M
	{
		HazelCamera Camera;
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

	class SceneH2M;
	struct MeshComponentH2M;

	class SceneRendererH2M : public RefCounted
	{
	public:
		SceneRendererH2M(Ref<SceneH2M> scene, SceneRendererSpecificationH2M specification = SceneRendererSpecificationH2M{});

		static void Init();
		static void Shutdown();

		void SetScene(Ref<SceneH2M> scene);

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(SceneH2M* scene, const SceneRendererCameraH2M& camera);
		static void EndScene();
		void UpdateHBAOData();

		static void SubmitMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent);
		static void SubmitSelectedMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent);

		static void SubmitMesh(Ref<MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<HazelMaterial> overrideMaterial = Ref<HazelMaterial>());
		static void SubmitSelectedMesh(Ref<MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<Texture2DH2M> GetFinalPassImage(); // previously: GetFinalColorBuffer

		static SceneRendererOptionsH2M& GetOptions();

		void SetLineWidth(float width);

		static void WaitForThreads();

private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
		static void ShadowMapPass();

	private:
		Ref<SceneH2M> m_Scene;
		SceneRendererSpecificationH2M m_Specification;
		Ref<RenderCommandBuffer> m_CommandBuffer;

	private:
		bool m_NeedsResize = false;
		bool m_Active = false;
		bool m_ResourcesCreated = false;

		float m_LineWidth = 2.0f;

	};

}
