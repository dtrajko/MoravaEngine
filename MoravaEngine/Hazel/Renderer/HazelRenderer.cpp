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
		glLineWidth(thickness);
	}

	const Scope<ShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data.m_ShaderLibrary;
	}

	void HazelRenderer::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		RendererAPI::DrawIndexed(count, type, depthTest);
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

	RenderCommandQueue& HazelRenderer::GetRenderCommandQueue()
	{
		return s_Data.m_CommandQueue;
	}

	void HazelRenderer::DrawAABB(Mesh* mesh, const glm::mat4& transform, glm::vec4& color)
	{
		for (Hazel::Submesh* submesh : ((Hazel::MeshAnimPBR*)mesh)->GetSubmeshes())
		{
			auto& aabb = submesh->BoundingBox;
			const auto& aabbTransform = transform * submesh->Transform;
			DrawAABB(aabb, aabbTransform, color);
		}
	}

	void HazelRenderer::DrawAABB(const Hazel::AABB& aabb, const glm::mat4& transform, glm::vec4& color)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4{ aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f},
			transform * glm::vec4{ aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f},
			transform * glm::vec4{ aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f},
			transform * glm::vec4{ aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f},

			transform * glm::vec4{ aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f},
			transform * glm::vec4{ aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f},
			transform * glm::vec4{ aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f},
			transform * glm::vec4{ aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f}
		};

		for (uint32_t i = 0; i < 4; i++) {
			Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);
		}

		for (uint32_t i = 0; i < 4; i++) {
			Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);
		}

		for (uint32_t i = 0; i < 4; i++) {
			Renderer2D::DrawLine(corners[i], corners[i + 4], color);
		}
	}

}
