#include "HazelRenderer.h"

#include "Hazel/Core/Assert.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "Renderer/RendererBasic.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderer.h"
// #include "Hazel/Platform/Vulkan/VulkanRenderer.h"

namespace Hazel {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

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

	void RendererAPI::SetAPI(RendererAPIType api)
	{
		// TODO: make sure this is called at a valid time
		s_CurrentRendererAPI = api;
	}

	struct RendererData
	{
		RendererConfig Config;

		Ref<HazelShaderLibrary> m_ShaderLibrary;

		Ref<HazelTexture2D> WhiteTexture;
		Ref<HazelTextureCube> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;

		// Obsolete properties
		Ref<VertexBuffer> m_FullscreenQuadVertexBuffer;
		Ref<IndexBuffer> m_FullscreenQuadIndexBuffer;
		Ref<Pipeline> m_FullscreenQuadPipeline;
	};

	static RendererData* s_Data = nullptr;
	static RenderCommandQueue* s_CommandQueue = nullptr;

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

	void HazelRenderer::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();
		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		// Tests loading shaders

		// HazelShader::Create("assets/shaders/Renderer2D.glsl"); // OK

		// HazelShader::Create("assets/shaders/Grid.glsl"); // NOT OK
		// HazelShader::Create("assets/shaders/Skybox.glsl"); // NOT OK
		// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl"); // NOT OK
		// HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Renderer2D.glsl"); // NOT OK

		//	// Compute shaders
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentMipFilter.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EquirectangularToCubeMap.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentIrradiance.glsl");
		//	
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Grid.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/SceneComposite.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
		//	//Renderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Anim.glsl");
		//	//Renderer::GetShaderLibrary()->Load("assets/shaders/Outline.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Texture.glsl");
		//	HazelRenderer::GetShaderLibrary()->Load("assets/shaders/ShadowMap.glsl");

		// Compile shaders
		HazelRenderer::WaitAndRender();

		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture = HazelTexture2D::Create(HazelImageFormat::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = HazelTextureCube::Create(HazelImageFormat::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		s_RendererAPI->Init();
		SceneRenderer::Init();
	}

	void HazelRenderer::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRenderer::Shutdown();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

	RendererCapabilities& HazelRenderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	Ref<HazelShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void HazelRenderer::WaitAndRender()
	{
		s_CommandQueue->Execute();
	}

	void HazelRenderer::BeginRenderPass(Ref<RenderPass> renderPass, bool clear)
	{
		HZ_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		s_RendererAPI->BeginRenderPass(renderPass);
	}

	void HazelRenderer::EndRenderPass()
	{
		s_RendererAPI->EndRenderPass();
	}

	void HazelRenderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void HazelRenderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void HazelRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
		s_RendererAPI->SetSceneEnvironment(environment, shadow);
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

	void HazelRenderer::SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		/****
		bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlag::DepthTest);
			cullFace = !material->GetFlag(MaterialFlag::TwoSided);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		s_Data->m_FullscreenQuadPipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		Renderer::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
		****/
	}

	void HazelRenderer::SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterial> materialoverrideMaterial)
	{
		Log::GetLogger()->warn("HazelRenderer::SubmitMesh Method not implemented!");
	}

	void HazelRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
	{
		s_RendererAPI->SubmitFullscreenQuad(pipeline, material);
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
#endif

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

	Ref<HazelTexture2D> HazelRenderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<HazelTextureCube> HazelRenderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}

	Ref<Environment> HazelRenderer::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RenderCommandQueue& HazelRenderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}

	RendererConfig& HazelRenderer::GetConfig()
	{
		return s_Data->Config;
	}


#if 0

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

#endif

}
