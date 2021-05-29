#include "HazelRenderer.h"

#include "Hazel/Core/Assert.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"
#include "RendererAPI.h"

#include "Renderer/RendererBasic.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"


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

	// static RenderCommandQueue* s_CommandQueue = nullptr;
	// static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	void HazelRenderer::Init()
	{
		s_Data.m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();
		// OPENGL ONLY - HazelRenderer::Submit([]() { RendererAPI::Init(); });

		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Grid.glsl");
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/SceneComposite.glsl");
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelSimple.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Outline.glsl");
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Texture.glsl");
		}

#if NOT_YET
		SceneRenderer::Init();

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
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data.m_FullscreenQuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		Renderer2D::Init();
#endif
	}

	Ref<HazelShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data.m_ShaderLibrary;
	}

	void HazelRenderer::Clear()
	{
		// HazelRenderer::Submit([]()
		// {
		// });
		{
			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}

	void HazelRenderer::Clear(float r, float g, float b, float a)
	{
		// HazelRenderer::Submit([=]()
		// {
		// });
		{
			RendererAPI::Clear(r, g, b, a);
		}
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
		// HazelRenderer::Submit([=]() {
		// });
		{
			RendererAPI::DrawIndexed(count, type, depthTest);
		}
	}

	void HazelRenderer::SetLineThickness(float thickness)
	{
		Log::GetLogger()->warn("HazelRenderer::SetLineThickness({0}): method not implemented!", thickness);

		// HazelRenderer::Submit([=]() {
		// });

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
			// HazelRenderer::Submit([=]()
			// {
			// });
			{
				RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			}
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
			material->Bind();
			depthTest = material->GetFlag(HazelMaterialFlag::DepthTest);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data.m_FullscreenQuadVertexBuffer->Bind();
		s_Data.m_FullscreenQuadPipeline->Bind();
		s_Data.m_FullscreenQuadIndexBuffer->Bind();
		HazelRenderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void HazelRenderer::SubmitFullscreenQuad(/* Ref<Pipeline> pipeline, */Ref<HazelMaterial> material)
	{
		bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(HazelMaterialFlag::DepthTest);
		}

		s_Data.m_FullscreenQuadVertexBuffer->Bind();
		s_Data.m_FullscreenQuadPipeline->Bind();
		s_Data.m_FullscreenQuadIndexBuffer->Bind();
		HazelRenderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void HazelRenderer::SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterialInstance> overrideMaterial)
	{
		// auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterialInstance();
		// auto shader = material->GetShader();
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
					mesh->m_MeshShader->SetMat4(uniformName, mesh->m_BoneTransforms[i]);
				}
			}

			auto transformUniform = transform * submesh.Transform;
			shader->SetUniformBuffer("Transform", &transformUniform, sizeof(glm::mat4));

			// HazelRenderer::Submit([submesh, material]()
			// {
			// });
			{
				if (material->GetFlag(HazelMaterialFlag::DepthTest))
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
			}
		}
	}

	void HazelRenderer::DrawAABB(Ref<HazelMesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Submesh& submesh : mesh->m_Submeshes)
		{
			auto& aabb = submesh.BoundingBox;
			auto aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void HazelRenderer::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[i + 4], color);
	}

	RenderCommandQueue& HazelRenderer::GetRenderCommandQueue()
	{
		return s_Data.m_CommandQueue;
	}

	// ---------------------------------------------------------------

	RenderAPICapabilities& HazelRenderer::GetCapabilities()
	{
		return RendererAPI::GetCapabilities();
	}

	// Used by OpenGLRenderer
	void HazelRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
		// s_RendererAPI->SetSceneEnvironment(environment, shadow);
	}

	// Used by OpenGLRenderer
	Ref<Environment> HazelRenderer::GetEmptyEnvironment()
	{
		// return s_Data.EmptyEnvironment;
		return Ref<Environment>();
	}


	struct ShaderDependencies
	{
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<HazelMaterial>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void HazelRenderer::RegisterShaderDependency(Ref<HazelShader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void HazelRenderer::RegisterShaderDependency(Ref<HazelShader> shader, Ref<HazelMaterial> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void HazelRenderer::OnShaderReloaded(size_t hash)
	{
		if (s_ShaderDependencies.find(hash) != s_ShaderDependencies.end())
		{
			auto& dependencies = s_ShaderDependencies.at(hash);
			for (auto& pipeline : dependencies.Pipelines)
			{
				pipeline->Invalidate();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::OpenGL: return new OpenGLRenderer();
		// case RendererAPIType::Vulkan: return new VulkanRenderer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<RendererContext> HazelRenderer::GetContext()
	{
		return Application::Get()->GetWindow()->GetRenderContext();
	}

	void HazelRenderer::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRenderer::Shutdown();
	}


#if 0

	void Renderer::SubmitFullscreenQuad(Ref<Material> material)
	{
		// Retrieve pipeline from cache
		auto& shader = material->GetShader();
		auto hash = shader->GetHash();
		if (s_PipelineCache.find(hash) == s_PipelineCache.end())
		{
			// Create pipeline
			PipelineSpecification spec = s_Data.m_FullscreenQuadPipelineSpec;
			spec.Shader = shader;
			spec.DebugName = "Renderer-FullscreenQuad-" + shader->GetName();
			s_PipelineCache[hash] = Pipeline::Create(spec);
		}

		auto& pipeline = s_PipelineCache[hash];

		bool depthTest = true;
		bool cullFace = true;
		if (material)
		{
			// material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);
		}

		s_Data.FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data.FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void HazelRenderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void HazelRenderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> HazelRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	void HazelRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void HazelRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

	void HazelRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	Ref<HazelTexture2D> HazelRenderer::GetWhiteTexture()
	{
		return s_Data.WhiteTexture;
	}

	Ref<HazelTextureCube> HazelRenderer::GetBlackCubeTexture()
	{
		return s_Data.BlackCubeTexture;
	}

	RendererConfig& HazelRenderer::GetConfig()
	{
		return s_Data.Config;
	}
#endif

}


void Hazel::Utils::DumpGPUInfo()
{
	auto& caps = HazelRenderer::GetCapabilities();
	Log::GetLogger()->trace("GPU Info:");
	Log::GetLogger()->trace("  Vendor: {0}", caps.Vendor);
	Log::GetLogger()->trace("  Device: {0}", caps.Device);
	Log::GetLogger()->trace("  Version: {0}", caps.Version);
}
