#include "RendererH2M.h"

#include "H2M/Core/Assert.h"
#include "H2M/Platform/OpenGL/OpenGLRenderer.h"
#include "H2M/Platform/Vulkan/VulkanComputePipeline.h"
#include "H2M/Renderer/PipelineCompute.h"
#include "H2M/Renderer/Renderer2D.h"
#include "H2M/Renderer/RendererAPI.h"
#include "H2M/Renderer/SceneRenderer.h"

#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Renderer/SceneRendererVulkan.h"

#include "Renderer/RendererBasic.h"
#include "Platform/DX11/DX11Renderer.h"


namespace H2M {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<PipelineCompute>> ComputePipelines;
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<HazelMaterial>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void RendererH2M::RegisterShaderDependency(Ref<HazelShader> shader, Ref<PipelineCompute> computePipeline)
	{
		s_ShaderDependencies[shader->GetHash()].ComputePipelines.push_back(computePipeline);
	}

	void RendererH2M::RegisterShaderDependency(Ref<HazelShader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void RendererH2M::RegisterShaderDependency(Ref<HazelShader> shader, Ref<HazelMaterial> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void RendererH2M::OnShaderReloaded(size_t hash)
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

	uint32_t RendererH2M::GetCurrentFrameIndex()
	{
		return Application::Get()->GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	struct RendererData
	{
		RendererConfig Config;

		Ref<RenderPass> m_ActiveRenderPass;
		RenderCommandQueue m_CommandQueue;
		Ref<HazelShaderLibrary> m_ShaderLibrary;

		Ref<VertexBuffer> FullscreenQuadVertexBuffer; // TODO: remove from RendererH2M
		Ref<IndexBuffer> FullscreenQuadIndexBuffer;   // TODO: remove from RendererH2M
		Ref<Pipeline> FullscreenQuadPipeline;         // TODO: remove from RendererH2M
		PipelineSpecification FullscreenQuadPipelineSpec;

		Ref<Texture2DH2M> WhiteTexture;
		Ref<Texture2DH2M> BlackTexture;
		Ref<Texture2DH2M> BRDFLutTexture;
		Ref<TextureCubeH2M> BlackCubeTexture;
		Ref<Environment> EmptyEnvironment;
	};

	static RendererData* s_Data = nullptr;
	static RenderCommandQueue* s_CommandQueue = nullptr;

	// static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;
	static RenderCommandQueue s_ResourceFreeQueue[3];

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL: return new OpenGLRenderer();
			case RendererAPIType::Vulkan: return new VulkanRendererH2M();
			case RendererAPIType::DX11:   return new DX11Renderer();
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void RendererH2M::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();

		// Make sure we don't have more frames in flight than swapchain images
		RendererH2M::GetConfig().FramesInFlight = glm::min<uint32_t>(RendererH2M::GetConfig().FramesInFlight, Application::Get()->GetWindow()->GetSwapChain().GetImageCount());

		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		//...

		// Compute shaders
		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			// RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/ClearCubeMap.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl");

			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Grid.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/SceneComposite.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Static.glsl");
			// RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Anim.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Outline.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
			RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/Texture.glsl");
		}

		// Compile shaders
		RendererH2M::WaitAndRender();
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
		s_Data->WhiteTexture = Texture2DH2M::Create(ImageFormatH2M::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCubeH2M::Create(ImageFormatH2M::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		//...
	}

	RendererCapabilities& RendererH2M::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	void RendererH2M::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		// RendererH2M::Submit([=]() {});
		{
			RendererAPI::DrawIndexed(count, type, depthTest);
		}
	}

	Ref<HazelShaderLibrary>& RendererH2M::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	//	void RendererH2M::Clear()
	//	{
	//		// RendererH2M::Submit([]() {});
	//		{
	//			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
	//		}
	//	}

	//	void RendererH2M::Clear(float r, float g, float b, float a)
	//	{
	//	}

	void RendererH2M::WaitAndRender()
	{
		s_Data->m_CommandQueue.Execute();
	}

	void RendererH2M::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		// RendererH2M::Submit([=]() {});
		{
			// RendererAPI::Clear(r, g, b, a);
		}
	}

	void RendererH2M::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndRenderPass(renderCommandBuffer);

		/**** BEGIN the old version of the method ****

		HZ_CORE_ASSERT(s_Data->m_ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		s_Data->m_ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
		s_Data->m_ActiveRenderPass = nullptr;

		/**** END the old version of the method ****/
	}

	//	void RendererH2M::ClearMagenta()
	//	{
	//		Clear(1, 0, 1);
	//	}

	//	void RendererH2M::SetClearColor(float r, float g, float b, float a)
	//	{
	//	}

	// Used by OpenGLRenderer
	void RendererH2M::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<HazelImage2D> shadow, Ref<HazelImage2D> linearDepth)
	{
		s_RendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow, linearDepth);
	}

	//	void RendererH2M::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	//	{
	//		// RendererH2M::Submit([=]() {});
	//		{
	//			RendererAPI::DrawIndexed(count, type, depthTest);
	//		}
	//	}

	//	void RendererH2M::SetLineThickness(float thickness)
	//	{
	//		Log::GetLogger()->warn("RendererH2M::SetLineThickness({0}): method not implemented!", thickness);
	//	
	//		// RendererH2M::Submit([=]() {});
	//		{
	//			RendererAPI::SetLineThickness(thickness);
	//		}
	//	}

	Ref<TextureCubeH2M> RendererH2M::CreatePreethamSky(float turbidity, float azimuth, float inclination)
	{
		// HZ_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		/**** BEGIN the new code ****/
		{
			// s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
			// s_RendererAPI->BeginRenderPass(renderPass);
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
				// RendererH2M::Submit([=]() {});
				{
					RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
				}
			}
		}
		/**** END The obsolete code moved to OpenGLRenderer ****/

		return Ref<TextureCubeH2M>();
	}

	void RendererH2M::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform)
	{
	}

	void RendererH2M::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material, Buffer additionalUniforms)
	{
	}

	void RendererH2M::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
	}

	void RendererH2M::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
	}

	void RendererH2M::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
	{
	}

	void RendererH2M::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		s_RendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, uniformBufferSet, material, vertexShaderOverrides, fragmentShaderOverrides);
	}

	void RendererH2M::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
	{
	}

	void RendererH2M::DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
	{
	}

	void RendererH2M::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount)
	{
	}

	std::pair<Ref<TextureCubeH2M>, Ref<TextureCubeH2M>> RendererH2M::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	void RendererH2M::SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform)
	{
	}

	void RendererH2M::SubmitMesh(Ref<MeshH2M> mesh, const glm::mat4& transform, Ref<HazelMaterialInstance> overrideMaterial)
	{
		// auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterialInstance();
		// auto shader = material->GetShader();
		// TODO: Sort this out
		mesh->GetVertexBuffer()->Bind();
		mesh->GetPipeline()->Bind();
		mesh->GetIndexBuffer()->Bind();

		auto& materials = mesh->GetMaterials();
		for (Ref<SubmeshH2M> submesh : mesh->GetSubmeshes())
		{
			// Material
			auto material = overrideMaterial ? overrideMaterial : materials[submesh->MaterialIndex];
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

			auto transformUniform = transform * submesh->Transform;
			shader->SetUniformBuffer("Transform", &transformUniform, sizeof(glm::mat4));

			// RendererH2M::Submit([submesh, material]() {});
			{
				if (material->GetFlag(HazelMaterialFlag::DepthTest))
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
			}
		}
	}

	void RendererH2M::SubmitMeshWithShader(Ref<MeshH2M> mesh, const glm::mat4& transform, Ref<HazelShader> shader)
	{
		mesh->GetVertexBuffer()->Bind();
		mesh->GetPipeline()->Bind();
		mesh->GetIndexBuffer()->Bind();

		for (Ref<SubmeshH2M> submesh : mesh->GetSubmeshes())
		{
			if (mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->GetBoneTransforms().size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					shader->SetMat4(uniformName, mesh->GetBoneTransforms()[i]);
				}
			}
			shader->SetMat4("u_Transform", transform * submesh->Transform);

			// RendererH2M::Submit([submesh]() {});
			glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
		}
	}

	void RendererH2M::DrawAABB(Ref<MeshH2M> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Ref<SubmeshH2M> submesh : mesh->GetSubmeshes())
		{
			auto& aabb = submesh->BoundingBox;
			auto aabbTransform = transform * submesh->Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void RendererH2M::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
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

	RenderCommandQueue& RendererH2M::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}

	void RendererH2M::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
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
		s_Data->FullscreenQuadPipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();

		// RendererH2M::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void RendererH2M::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<HazelImage2D> image)
	{
		s_RendererAPI->ClearImage(renderCommandBuffer, image);
	}

	Ref<RendererContext> RendererH2M::GetContext()
	{
		return Application::Get()->GetWindow()->GetRenderContext();
	}

	void RendererH2M::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRenderer::Shutdown();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

	void RendererH2M::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void RendererH2M::EndFrame()
	{
		s_RendererAPI->EndFrame();
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

	void RendererH2M::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

#endif

	void RendererH2M::RenderMesh(Ref<Pipeline> pipeline, Ref<MeshH2M> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void RendererH2M::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	Ref<Texture2DH2M> RendererH2M::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2DH2M> RendererH2M::GetBlackTexture()
	{
		return Ref<Texture2DH2M>();
	}

	Ref<Texture2DH2M> RendererH2M::GetBRDFLutTexture()
	{
		return Ref<Texture2DH2M>();
	}

	// disabled in some versions of Hazel-dev
	Ref<TextureCubeH2M> RendererH2M::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}

	Ref<Environment> RendererH2M::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RendererAPI* RendererH2M::GetRendererAPI()
	{
		return s_RendererAPI;
	}

	RenderCommandQueue& RendererH2M::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}

	RendererConfigH2M& RendererH2M::GetConfig()
	{
		static RendererConfigH2M config;
		return config;
	}

}
