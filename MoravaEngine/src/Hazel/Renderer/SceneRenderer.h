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
		bool ShowBoundingBoxes = false;
	};

	struct SceneRendererCamera
	{
		Hazel::HazelCamera Camera;
		glm::mat4 ViewMatrix;
		float Near, Far;
		float FOV;
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
		static void SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = Ref<Material>());
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

	};

}
