#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Core/Ref.h"

#include "HazelLegacy/Scene/ComponentsHazelLegacy.h"

#include <glm/glm.hpp>


namespace Hazel {

	struct SceneRendererOptionsHazelLegacy
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

	struct SceneRendererCameraHazelLegacy
	{
		HazelCamera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
	};

	struct BloomSettingsHazelLegacy
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.1f;
		float UpsampleScale = 1.0f;
		float Intensity = 1.0f;
		float DirtIntensity = 1.0f;
	};

	struct SceneRendererSpecificationHazelLegacy
	{
		bool SwapChainTarget = false;
	};

	class SceneHazelLegacy;
	struct MeshComponentHazelLegacy;

	class SceneRendererHazelLegacy : public RefCounted
	{
	public:
		SceneRendererHazelLegacy(Ref<SceneHazelLegacy> scene, SceneRendererSpecificationHazelLegacy specification = SceneRendererSpecificationHazelLegacy{});

		static void Init();
		static void Shutdown();

		void SetScene(Ref<SceneHazelLegacy> scene);

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(SceneHazelLegacy* scene, const SceneRendererCameraHazelLegacy& camera);
		static void EndScene();
		void UpdateHBAOData();

		static void SubmitMesh(MeshComponentHazelLegacy meshComponent, TransformComponentHazelLegacy transformComponent);
		static void SubmitSelectedMesh(MeshComponentHazelLegacy meshComponent, TransformComponentHazelLegacy transformComponent);

		static void SubmitMesh(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<HazelMaterial> overrideMaterial = Ref<HazelMaterial>());
		static void SubmitSelectedMesh(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<HazelTexture2D> GetFinalPassImage(); // previously: GetFinalColorBuffer

		static SceneRendererOptionsHazelLegacy& GetOptions();

		void SetLineWidth(float width);

		static void WaitForThreads();

private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
		static void ShadowMapPass();

	private:
		Ref<SceneHazelLegacy> m_Scene;
		SceneRendererSpecificationHazelLegacy m_Specification;
		Ref<RenderCommandBuffer> m_CommandBuffer;

	private:
		bool m_NeedsResize = false;
		bool m_Active = false;
		bool m_ResourcesCreated = false;

		float m_LineWidth = 2.0f;

	};

}
