#pragma once

#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Scene/HazelScene.h"


namespace Hazel {

	class SceneRendererVulkan
	{
	public:
		SceneRendererVulkan();
		~SceneRendererVulkan();

		static void Init();
		static void SetViewportSize(uint32_t width, uint32_t height);
		static void BeginScene(const HazelScene* scene, const SceneRendererCamera& camera);
		static void EndScene();

		static void SubmitMesh(MeshComponent meshComponent, TransformComponent transformComponent);
		static void SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Material> overrideMaterial = Ref<Material>());
		static void SubmitSelectedMesh(MeshComponent meshComponent, TransformComponent transformComponent);
		static void SubmitSelectedMesh(Ref<HazelMesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static void SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterial> overrideMaterial);
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static SceneRendererOptions& GetOptions();

	};

}
