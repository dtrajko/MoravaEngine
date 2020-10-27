#include "HazelRenderer.h"

#include "../Core/Assert.h"
#include "RenderPass.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "MeshAnimPBR.h"
#include "Renderer2D.h"

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

	void HazelRenderer::ClearMagenta()
	{
	}

	void HazelRenderer::Init()
	{
	}

	void HazelRenderer::SetLineThickness(float thickness)
	{
	}

	const Scope<ShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data.m_ShaderLibrary;
	}

	void HazelRenderer::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		HazelRenderer::Submit([=]() {
			RendererAPI::DrawIndexed(count, type, depthTest);
		});
	}

	void HazelRenderer::WaitAndRender()
	{
		s_Data.m_CommandQueue.Execute();
	}

	void HazelRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass, bool clear)
	{
		HZ_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		// TODO: Convert all of this into a render command buffer
		s_Data.m_ActiveRenderPass = renderPass;

		renderPass->GetSpecification().TargetFramebuffer->Bind();
		if (clear)
		{
			const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification().ClearColor;
			HazelRenderer::Submit([=]() {
				RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			});
		}
	}

	void HazelRenderer::EndRenderPass()
	{
		HZ_CORE_ASSERT(s_Data.m_ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		s_Data.m_ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
		s_Data.m_ActiveRenderPass = nullptr;
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
