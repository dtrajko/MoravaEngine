/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/SceneRendererH2M.h"
#include "H2M/Scene/SceneH2M.h"


namespace H2M
{

	class SceneRendererVulkanH2M
	{
	public:
		static void Init();
		static void Shutdown();

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const SceneH2M* scene, const SceneRendererCameraH2M& camera);
		static void EndScene();

		static void SubmitMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent);
		static void SubmitSelectedMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent);

		static void SubmitMesh(Ref<MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<MaterialH2M> overrideMaterial = Ref<MaterialH2M>());
		static void SubmitSelectedMesh(Ref<MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static SceneRendererOptions& GetOptions();
private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
		static void BloomBlurPass();
		static void ShadowMapPass();


	};

}
