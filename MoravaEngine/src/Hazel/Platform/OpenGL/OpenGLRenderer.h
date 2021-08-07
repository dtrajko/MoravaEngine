#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RendererCapabilities.h"


namespace Hazel {

	class OpenGLRenderer : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(const Ref<RenderPass>& renderPass) override;
		virtual void EndRenderPass() override;
		virtual void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material) override;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow) override;
		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) override;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) override;

		virtual RendererCapabilities& GetCapabilities() override;

	};

}
