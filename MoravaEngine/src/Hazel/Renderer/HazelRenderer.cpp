#include "HazelRenderer.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanComputePipeline.h"
#include "Hazel/Renderer/PipelineCompute.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Renderer/SceneRendererVulkan.h"

#include "Renderer/RendererBasic.h"
#include "Platform/DX11/DX11Renderer.h"


namespace Hazel {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<PipelineCompute>> ComputePipelines;
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

			for (auto& computePipeline : dependencies.ComputePipelines)
			{
				computePipeline.As<VulkanComputePipeline>()->CreatePipeline();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		RendererConfig Config;

		Ref<RenderPass> m_ActiveRenderPass;
		RenderCommandQueue m_CommandQueue;
		Ref<HazelShaderLibrary> m_ShaderLibrary;

		Ref<VertexBuffer> FullscreenQuadVertexBuffer; // TODO: remove from HazelRenderer
		Ref<IndexBuffer> FullscreenQuadIndexBuffer;   // TODO: remove from HazelRenderer
		Ref<Pipeline> FullscreenQuadPipeline;         // TODO: remove from HazelRenderer
		PipelineSpecification FullscreenQuadPipelineSpec;

		Ref<HazelTexture2D> WhiteTexture;
		Ref<HazelTexture2D> BlackTexture;
		Ref<HazelTexture2D> BRDFLutTexture;
		Ref<HazelTextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;
	};

	static RendererData* s_Data = nullptr;
	static RenderCommandQueue* s_CommandQueue = nullptr;

	// static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL: return new OpenGLRenderer();
			case RendererAPIType::Vulkan: return new VulkanRenderer();
			case RendererAPIType::DX11:   return new DX11Renderer();
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void HazelRenderer::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();
		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		//...

		// Compute shaders
		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/ClearCubeMap.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EquirectangularToCubeMap.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentMipFilter.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentIrradiance.glsl");

			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Grid.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/SceneComposite.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
			// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Anim.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Outline.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
			HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Texture.glsl");
		}

		// Compile shaders
		HazelRenderer::WaitAndRender();
		SceneRenderer::Init();

		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			SceneRendererVulkan::Init();
		}

		std::function<void()> initFunc;

		switch (RendererAPI::Current())
		{
		case RendererAPIType::OpenGL:
			initFunc; // = OpenGLRenderer::Init;
			break;
		case RendererAPIType::Vulkan:
			initFunc; // = VulkanRenderer::Init;
			break;
		}

		// initFunc();

		s_RendererAPI->Init(); // this method is currently OpenGL-specific

		// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2;
		float height = 2;
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

		s_Data->FullscreenQuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
		s_Data->FullscreenQuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		// ...

		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture = HazelTexture2D::Create(HazelImageFormat::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = HazelTextureCube::Create(HazelImageFormat::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		//...

	}

	RendererCapabilities& HazelRenderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<HazelShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	/**** BEGIN to be removed from HazelRenderer ****/
	void HazelRenderer::Clear()
	{
		// HazelRenderer::Submit([]() {});
		{
			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void HazelRenderer::Clear(float r, float g, float b, float a)
	{
		// HazelRenderer::Submit([=]() {});
		{
			RendererAPI::Clear(r, g, b, a);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void HazelRenderer::ClearMagenta()
	{
		Clear(1, 0, 1);
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void HazelRenderer::SetClearColor(float r, float g, float b, float a)
	{
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void HazelRenderer::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		// HazelRenderer::Submit([=]() {});
		{
			RendererAPI::DrawIndexed(count, type, depthTest);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void HazelRenderer::SetLineThickness(float thickness)
	{
		Log::GetLogger()->warn("HazelRenderer::SetLineThickness({0}): method not implemented!", thickness);

		// HazelRenderer::Submit([=]() {});
		{
			RendererAPI::SetLineThickness(thickness);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	void HazelRenderer::WaitAndRender()
	{
		s_Data->m_CommandQueue.Execute();
	}

	void HazelRenderer::BeginRenderPass(Ref<RenderPass> renderPass, bool clear)
	{
		HZ_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		/**** BEGIN the new code ****/
		{
			// s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
			s_RendererAPI->BeginRenderPass(renderPass);
		}
		/**** END the new code ****/

		/**** BEGIN The obsolete code moved to OpenGLRenderer ****
		{
			// TODO: Convert all of this into a render command buffer
			s_Data->m_ActiveRenderPass = renderPass;

			renderPass->GetSpecification().TargetFramebuffer->Bind();
			if (clear)
			{
				const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification().ClearColor;
				// HazelRenderer::Submit([=]() {});
				{
					RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
				}
			}
		}
		/**** END The obsolete code moved to OpenGLRenderer ****/
	}

	void HazelRenderer::EndRenderPass()
	{
		s_RendererAPI->EndRenderPass();

		/**** BEGIN the old version of the method ****

		HZ_CORE_ASSERT(s_Data->m_ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		s_Data->m_ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
		s_Data->m_ActiveRenderPass = nullptr;

		/**** END the old version of the method ****/
	}

	// Used by OpenGLRenderer
	void HazelRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
		s_RendererAPI->SetSceneEnvironment(environment, shadow);
	}

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> HazelRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
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

		s_Data->FullscreenQuadVertexBuffer->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();
		s_Data->FullscreenQuadPipeline->Bind();

		HazelRenderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void HazelRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(pipeline, material);

		/**** BEGIN the old version of the method ****

		// Retrieve pipeline from cache
		auto& shader = material->GetShader();
		auto hash = shader->GetHash();
		if (s_PipelineCache.find(hash) == s_PipelineCache.end())
		{
			// Create pipeline
			PipelineSpecification spec = s_Data->FullscreenQuadPipelineSpec;
			spec.Shader = shader;
			spec.DebugName = "Renderer-FullscreenQuad-" + shader->GetName();
			s_PipelineCache[hash] = Pipeline::Create(spec);
		}

		auto& pipelineLocal = s_PipelineCache[hash];

		bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(HazelMaterialFlag::DepthTest);
		}

		s_Data->FullscreenQuadVertexBuffer->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();
		s_Data->FullscreenQuadPipeline->Bind();

		HazelRenderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);

		/**** END the old version of the method ****/
	}

	void HazelRenderer::SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterialInstance> overrideMaterial)
	{
		// auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterialInstance();
		// auto shader = material->GetShader();
		// TODO: Sort this out
		mesh->GetVertexBuffer()->Bind();
		mesh->GetIndexBuffer()->Bind();
		mesh->GetPipeline()->Bind();

		auto& materials = mesh->GetMaterials();
		for (Submesh& submesh : mesh->GetSubmeshes())
		{
			// Material
			auto material = overrideMaterial ? overrideMaterial : materials[submesh.MaterialIndex];
			auto shader = material->GetShader();
			material->Bind();

			if (false && mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->GetBoneTransforms().size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					mesh->GetMeshShader()->SetMat4(uniformName, mesh->GetBoneTransforms()[i]);
				}
			}

			auto transformUniform = transform * submesh.Transform;
			shader->SetUniformBuffer("Transform", &transformUniform, sizeof(glm::mat4));

			// HazelRenderer::Submit([submesh, material]() {});
			{
				if (material->GetFlag(HazelMaterialFlag::DepthTest))
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
			}
		}
	}

	void HazelRenderer::SubmitMeshWithShader(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelShader> shader)
	{
		mesh->GetVertexBuffer()->Bind();
		mesh->GetIndexBuffer()->Bind();
		mesh->GetPipeline()->Bind();

		for (Submesh& submesh : mesh->GetSubmeshes())
		{
			if (mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->GetBoneTransforms().size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					shader->SetMat4(uniformName, mesh->GetBoneTransforms()[i]);
				}
			}
			shader->SetMat4("u_Transform", transform * submesh.Transform);

			// HazelRenderer::Submit([submesh]() {});
			glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
		}
	}

	void HazelRenderer::DrawAABB(Ref<HazelMesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Submesh& submesh : mesh->GetSubmeshes())
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
		return s_Data->m_CommandQueue;
	}

	// ---------------------------------------------------------------

	// Used by OpenGLRenderer
	Ref<Environment> HazelRenderer::GetEmptyEnvironment()
	{
		// return s_Data->EmptyEnvironment;
		return Ref<Environment>();
	}

	uint32_t HazelRenderer::GetCurrentFrameIndex()
	{
		// return uint32_t();
		return Application::Get()->GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	Ref<RendererContext> HazelRenderer::GetContext()
	{
		return Application::Get()->GetWindow()->GetRenderContext();
	}

	void HazelRenderer::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRenderer::Shutdown();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
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
			PipelineSpecification spec = s_Data->m_FullscreenQuadPipelineSpec;
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

		s_Data->FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();
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

	void HazelRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

#endif

	void HazelRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void HazelRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	// disabled in some versions of Hazel-dev
	RendererConfig& HazelRenderer::GetConfig()
	{
		return s_Data->Config;
	}

	Ref<HazelTexture2D> HazelRenderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	// disabled in some versions of Hazel-dev
	Ref<HazelTextureCube> HazelRenderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}

	RendererAPI* HazelRenderer::GetRendererAPI()
	{
		return s_RendererAPI;
	}

}
