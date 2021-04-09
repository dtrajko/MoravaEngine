#pragma once

#include "Hazel/Scene/HazelScene.h"
#include "Hazel/Scene/Components.h"
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

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const HazelScene* scene, const SceneRendererCamera& camera);
		static void EndScene();

		// old?
		static void SubmitMesh(MeshComponent meshComponent, TransformComponent transformComponent);
		static void SubmitSelectedMesh(MeshComponent meshComponent, TransformComponent transformComponent);

		// new?
		static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = nullptr);
		static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<HazelTexture2D> GetFinalColorBuffer();
		
		// TODO: Temp
		static uint32_t GetFinalColorBufferRendererID();

		static SceneRendererOptions& GetOptions();
	private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
	};

}
