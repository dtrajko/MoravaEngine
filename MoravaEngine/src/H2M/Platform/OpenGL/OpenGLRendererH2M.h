/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererCapabilitiesH2M.h"


namespace H2M
{

	class OpenGLRendererH2M : public RendererAPI_H2M
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(const RefH2M<RenderPassH2M>& renderPass) override;
		virtual void EndRenderPass() override;
		virtual void SubmitFullscreenQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material) override;

		virtual void SetSceneEnvironment(RefH2M<EnvironmentH2M> environment, RefH2M<Image2D_H2M> shadow) override;

		virtual void RenderMesh(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material, const glm::mat4& transform) override;

		virtual void DrawIndexed(uint32_t indexCount, PrimitiveTypeH2M type, bool depthTest = true) override;
		virtual void DrawLines(RefH2M<VertexArrayH2M> vertexArray, uint32_t vertexCount) override;

		virtual void SetLineWidth(float width) override;

		virtual std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;

		virtual RendererCapabilitiesH2M& GetCapabilities() override;

	};

}
