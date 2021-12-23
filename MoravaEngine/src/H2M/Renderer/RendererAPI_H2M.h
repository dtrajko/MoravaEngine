/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/RendererCapabilitiesH2M.h"
#include "H2M/Renderer/RendererTypesH2M.h"
#include "H2M/Renderer/SceneEnvironmentH2M.h"
#include "H2M/Renderer/VertexArrayH2M.h"

#include <cstdint>
#include <string>


namespace H2M
{

	enum class RendererAPITypeH2M
	{
		None,
		Vulkan,
		OpenGL,
		DX11,
	};

	enum class PrimitiveTypeH2M
	{
		None = 0, Triangles, Lines
	};

	class PipelineH2M;
	class MaterialH2M;
	class MeshH2M;
	class RenderPassH2M;


	class RendererAPI_H2M
	{
	public:
		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void BeginRenderPass(const RefH2M<RenderPassH2M>& renderPass) = 0;
		virtual void EndRenderPass() = 0;
		virtual void SubmitFullscreenQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material) = 0;
		// virtual void SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform) = 0;

		virtual void SetSceneEnvironment(RefH2M<EnvironmentH2M> environment, RefH2M<Image2D_H2M> shadow) = 0;

		virtual void RenderMesh(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) = 0;
		virtual void RenderMeshWithoutMaterial(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) = 0;
		virtual void RenderQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material, const glm::mat4& transform) = 0;

		virtual void DrawIndexed(uint32_t indexCount, PrimitiveTypeH2M type, bool depthTest = true) = 0;
		virtual void DrawLines(RefH2M<VertexArrayH2M> vertexArray, uint32_t vertexCount) = 0;

		virtual void SetLineWidth(float width) = 0;

		virtual std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) = 0;

		virtual RendererCapabilitiesH2M& GetCapabilities() = 0;

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);
		static void SetLineThickness(float thickness);

		static RendererAPITypeH2M Current() { return s_CurrentRendererAPI; }
		static void SetAPI(RendererAPITypeH2M api);

	private:
		static void LoadRequiredAssets();

	private:
		inline static RendererAPITypeH2M s_CurrentRendererAPI = RendererAPITypeH2M::Vulkan;

	};

}
