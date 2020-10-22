#include "HazelRenderer.h"

#include "RenderPass.h"
#include "VertexArray.h"

#include "../../Log.h"


namespace Hazel {

	struct RendererData
	{
		Ref<RenderPass> m_ActiveRenderPass;
		RenderCommandQueue m_CommandQueue;
		Scope<ShaderLibrary> m_ShaderLibrary;
		Ref<VertexArray> m_FullscreenQuadVertexArray;
	};

	static RendererData s_Data;

	void HazelRenderer::Clear()
	{
	}

	void HazelRenderer::Clear(float r, float g, float b, float a)
	{
	}

	void HazelRenderer::SetClearColor(float r, float g, float b, float a)
	{
	}

	void HazelRenderer::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
	}

	void HazelRenderer::SetLineThickness(float thickness)
	{
	}

	void HazelRenderer::ClearMagenta()
	{
	}

	void HazelRenderer::Init()
	{
	}

	const Scope<ShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data.m_ShaderLibrary;
	}

	void HazelRenderer::WaitAndRender()
	{
	}

	void HazelRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass, bool clear)
	{
	}

	void HazelRenderer::EndRenderPass()
	{
	}

	void HazelRenderer::SubmitQuad(const Ref<HazelMaterialInstance>& material, const glm::mat4& transform)
	{
	}

	void HazelRenderer::SubmitFullscreenQuad(const Ref<HazelMaterialInstance>& material)
	{
	}

	void HazelRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform, const Ref<HazelMaterialInstance>& overrideMaterial)
	{
	}

	void HazelRenderer::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color)
	{
	}

	void HazelRenderer::DrawAABB(const Ref<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& color)
	{
	}

	RenderCommandQueue& HazelRenderer::GetRenderCommandQueue()
	{
		return s_Data.m_CommandQueue;
	}
}
