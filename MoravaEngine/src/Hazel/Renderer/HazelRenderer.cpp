#include "HazelRenderer.h"

#include "../Core/Assert.h"
#include "Renderer2D.h"
#include "SceneRenderer.h"


namespace Hazel {

	RendererAPIType RendererAPI::s_CurrentRendererAPI = RendererAPIType::OpenGL;

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		Ref<RenderPass> m_ActiveRenderPass;
		RenderCommandQueue m_CommandQueue;
		Ref<HazelShaderLibrary> m_ShaderLibrary;

		Ref<VertexBuffer> m_FullscreenQuadVertexBuffer;
		Ref<IndexBuffer> m_FullscreenQuadIndexBuffer;
		Ref<Pipeline> m_FullscreenQuadPipeline;
	};

	static RendererData s_Data;

	void HazelRenderer::Init()
	{
		s_Data.m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		Log::GetLogger()->debug("RendererBasic::GetSpirVEnabled(): {0}", RendererBasic::GetSpirVEnabled());

		if (RendererBasic::GetSpirVEnabled()) {
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Grid.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Anim.glsl");

			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Texture.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/ShadowMap.glsl");

		}

		// SceneRenderer::Init(); // TODO

		// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2, height = 2;

		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.1f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.1f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.1f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.1f);
		data[3].TexCoord = glm::vec2(0, 1);

		PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};
		s_Data.m_FullscreenQuadPipeline = Pipeline::Create(pipelineSpecification);

		s_Data.m_FullscreenQuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		s_Data.m_FullscreenQuadVertexBuffer->SetLayout(pipelineSpecification.Layout);

		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data.m_FullscreenQuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		Renderer2D::Init();
	}

	Ref<HazelShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data.m_ShaderLibrary;
	}

	void HazelRenderer::Clear()
	{
		HazelRenderer::Submit([]() {
			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
		});
	}

	void HazelRenderer::Clear(float r, float g, float b, float a)
	{
		HazelRenderer::Submit([=]() {
			RendererAPI::Clear(r, g, b, a);
		});
	}

	void HazelRenderer::ClearMagenta()
	{
		Clear(1, 0, 1);
	}

	void HazelRenderer::SetClearColor(float r, float g, float b, float a)
	{
	}

	void HazelRenderer::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			RendererAPI::DrawIndexed(count, type, depthTest);
		}
	}

	void HazelRenderer::SetLineThickness(float thickness)
	{
		HazelRenderer::Submit([=]() {
		});

		{
			RendererAPI::SetLineThickness(thickness);
		}
	}

	void HazelRenderer::WaitAndRender()
	{
		s_Data.m_CommandQueue.Execute();
	}

	void HazelRenderer::BeginRenderPass(Ref<RenderPass> renderPass, bool clear)
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

	void HazelRenderer::SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		bool depthTest = true;
		if (material)
		{
			// material->Bind(); // obsolete?
			depthTest = material->GetFlag(HazelMaterialFlag::DepthTest);

			auto shader = material->GetShader();
			shader->SetMat4("u_Transform", transform);
		}

		s_Data.m_FullscreenQuadVertexBuffer->Bind();
		s_Data.m_FullscreenQuadPipeline->Bind();
		s_Data.m_FullscreenQuadIndexBuffer->Bind();

		HazelRenderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void HazelRenderer::SubmitFullscreenQuad(Ref<HazelMaterial> material)
	{
		bool depthTest = true;
		if (material)
		{
			material->Bind(); // obsolete?
			depthTest = material->GetFlag(HazelMaterialFlag::DepthTest);
		}

		s_Data.m_FullscreenQuadVertexBuffer->Bind();
		s_Data.m_FullscreenQuadPipeline->Bind();
		s_Data.m_FullscreenQuadIndexBuffer->Bind();

		HazelRenderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	/**
	 * THIS METHOD IS NOT USED IN CURRENT MORAVA ENGINE WORKFLOW
	 */
	void HazelRenderer::SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterial> overrideMaterial)
	{
		auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterial();
		auto shader = material->GetShader();
		// TODO: Sort this out

		mesh->m_VertexBuffer->Bind();
		mesh->m_Pipeline->Bind();
		mesh->m_IndexBuffer->Bind();

		auto& materials = mesh->GetMaterials();
		for (Submesh& submesh : mesh->m_Submeshes)
		{
			// Material
			auto material = overrideMaterial ? overrideMaterial : materials[submesh.MaterialIndex];
			auto shader = material->GetShader();
			material->Bind();

			if (false && mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->m_BoneTransforms.size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					shader->SetMat4(uniformName, mesh->m_BoneTransforms[i]);
				}
			}

			auto transformUniform = transform * submesh.Transform;
			shader->SetUniformBuffer("Transform", &transformUniform, sizeof(glm::mat4));

			HazelRenderer::Submit([submesh, material]() {
			});

			if (material->GetFlag(HazelMaterialFlag::DepthTest)) {
				glEnable(GL_DEPTH_TEST);
			}
			else {
				glDisable(GL_DEPTH_TEST);
			}

			if (!material->GetFlag(HazelMaterialFlag::TwoSided)) {
				HazelRenderer::Submit([]() { glEnable(GL_CULL_FACE); });
			}
			else {
				HazelRenderer::Submit([]() { glDisable(GL_CULL_FACE); });
			}

			glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
		}
	}

	RenderCommandQueue& HazelRenderer::GetRenderCommandQueue()
	{
		return s_Data.m_CommandQueue;
	}

	void HazelRenderer::DrawAABB(const Ref<Mesh>& mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Hazel::Submesh& submesh : ((Hazel::HazelMesh*)mesh.Raw())->GetSubmeshes())
		{
			auto& aabb = submesh.BoundingBox;
			const auto& aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform, color);
		}
	}

	void HazelRenderer::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color)
	{
		//	Log::GetLogger()->debug("HazelRenderer::DrawAABB Min [ {0} {1} {2} ] Max [ {3} {4} {5} ]",
		//		aabb.Min.x, aabb.Min.y, aabb.Min.z, aabb.Max.x, aabb.Max.y, aabb.Max.z);

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
