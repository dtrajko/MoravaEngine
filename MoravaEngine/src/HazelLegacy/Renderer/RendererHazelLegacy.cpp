#include "RendererHazelLegacy.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanComputePipeline.h"
#include "Hazel/Renderer/PipelineCompute.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "HazelLegacy/Platform/Vulkan/VulkanRendererHazelLegacy.h"
#include "HazelLegacy/Renderer/SceneRendererVulkan.h"

#include "Renderer/RendererBasic.h"
#include "Platform/DX11/DX11Renderer.h"


namespace HazelLegacy {

	static std::unordered_map<size_t, Ref<Pipeline>> s_PipelineCache;

	static RendererAPI* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<Ref<PipelineCompute>> ComputePipelines;
		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<HazelMaterial>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void RendererHazelLegacy::RegisterShaderDependency(Ref<HazelShader> shader, Ref<PipelineCompute> computePipeline)
	{
		s_ShaderDependencies[shader->GetHash()].ComputePipelines.push_back(computePipeline);
	}

	void RendererHazelLegacy::RegisterShaderDependency(Ref<HazelShader> shader, Ref<Pipeline> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void RendererHazelLegacy::RegisterShaderDependency(Ref<HazelShader> shader, Ref<HazelMaterial> material)
	{
		s_ShaderDependencies[shader->GetHash()].Materials.push_back(material);
	}

	void RendererHazelLegacy::OnShaderReloaded(size_t hash)
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

	uint32_t RendererHazelLegacy::GetCurrentFrameIndex()
	{
		return Application::Get()->GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	struct RendererData
	{
		RendererConfig Config;

		Ref<RenderPass> m_ActiveRenderPass;
		RenderCommandQueue m_CommandQueue;
		Ref<HazelShaderLibrary> m_ShaderLibrary;

		Ref<VertexBuffer> FullscreenQuadVertexBuffer; // TODO: remove from RendererHazelLegacy
		Ref<IndexBuffer> FullscreenQuadIndexBuffer;   // TODO: remove from RendererHazelLegacy
		Ref<Pipeline> FullscreenQuadPipeline;         // TODO: remove from RendererHazelLegacy
		PipelineSpecification FullscreenQuadPipelineSpec;

		Ref<Texture2DHazelLegacy> WhiteTexture;
		Ref<Texture2DHazelLegacy> BlackTexture;
		Ref<Texture2DHazelLegacy> BRDFLutTexture;
		Ref<TextureCubeHazelLegacy> BlackCubeTexture;
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
			case RendererAPIType::Vulkan: return new VulkanRendererHazelLegacy();
			case RendererAPIType::DX11:   return new DX11Renderer();
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void RendererHazelLegacy::Init()
	{
		s_Data = new RendererData();
		s_CommandQueue = new RenderCommandQueue();

		// Make sure we don't have more frames in flight than swapchain images
		RendererHazelLegacy::GetConfig().FramesInFlight = glm::min<uint32_t>(RendererHazelLegacy::GetConfig().FramesInFlight, Application::Get()->GetWindow()->GetSwapChain().GetImageCount());

		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		//...

		// Compute shaders
		if (RendererAPI::Current() == RendererAPIType::Vulkan)
		{
			// RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/ClearCubeMap.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/EquirectangularToCubeMap.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentMipFilter.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/EnvironmentIrradiance.glsl");

			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/Grid.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/SceneComposite.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Static.glsl");
			// RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/HazelPBR_Anim.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/Outline.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/Skybox.glsl");
			RendererHazelLegacy::GetShaderLibrary()->Load("Resources/Shaders/Texture.glsl");
		}

		// Compile shaders
		RendererHazelLegacy::WaitAndRender();
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
		s_Data->WhiteTexture = Texture2DHazelLegacy::Create(HazelImageFormat::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCubeHazelLegacy::Create(HazelImageFormat::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = Ref<Environment>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		//...
	}

	RendererCapabilities& RendererHazelLegacy::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	void RendererHazelLegacy::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		// RendererHazelLegacy::Submit([=]() {});
		{
			RendererAPI::DrawIndexed(count, type, depthTest);
		}
	}

	Ref<HazelShaderLibrary>& RendererHazelLegacy::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	//	void RendererHazelLegacy::Clear()
	//	{
	//		// RendererHazelLegacy::Submit([]() {});
	//		{
	//			RendererAPI::Clear(0.0f, 0.0f, 0.0f, 1.0f);
	//		}
	//	}

	//	void RendererHazelLegacy::Clear(float r, float g, float b, float a)
	//	{
	//	}

	void RendererHazelLegacy::WaitAndRender()
	{
		s_Data->m_CommandQueue.Execute();
	}

	void RendererHazelLegacy::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		// RendererHazelLegacy::Submit([=]() {});
		{
			// RendererAPI::Clear(r, g, b, a);
		}
	}

	void RendererHazelLegacy::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RendererAPI->EndRenderPass(renderCommandBuffer);

		/**** BEGIN the old version of the method ****

		HZ_CORE_ASSERT(s_Data->m_ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		s_Data->m_ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
		s_Data->m_ActiveRenderPass = nullptr;

		/**** END the old version of the method ****/
	}

	//	void RendererHazelLegacy::ClearMagenta()
	//	{
	//		Clear(1, 0, 1);
	//	}

	//	void RendererHazelLegacy::SetClearColor(float r, float g, float b, float a)
	//	{
	//	}

	// Used by OpenGLRenderer
	void RendererHazelLegacy::SetSceneEnvironment(Ref<SceneRenderer> sceneRenderer, Ref<Environment> environment, Ref<HazelImage2D> shadow, Ref<HazelImage2D> linearDepth)
	{
		s_RendererAPI->SetSceneEnvironment(sceneRenderer, environment, shadow, linearDepth);
	}

	//	void RendererHazelLegacy::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	//	{
	//		// RendererHazelLegacy::Submit([=]() {});
	//		{
	//			RendererAPI::DrawIndexed(count, type, depthTest);
	//		}
	//	}

	//	void RendererHazelLegacy::SetLineThickness(float thickness)
	//	{
	//		Log::GetLogger()->warn("RendererHazelLegacy::SetLineThickness({0}): method not implemented!", thickness);
	//	
	//		// RendererHazelLegacy::Submit([=]() {});
	//		{
	//			RendererAPI::SetLineThickness(thickness);
	//		}
	//	}

	Ref<TextureCubeHazelLegacy> RendererHazelLegacy::CreatePreethamSky(float turbidity, float azimuth, float inclination)
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
				// RendererHazelLegacy::Submit([=]() {});
				{
					RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
				}
			}
		}
		/**** END The obsolete code moved to OpenGLRenderer ****/

		return Ref<TextureCubeHazelLegacy>();
	}

	void RendererHazelLegacy::RenderMesh(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, Ref<MaterialTable> materialTable, const glm::mat4& transform)
	{
	}

	void RendererHazelLegacy::RenderMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Mesh> mesh, const glm::mat4& transform, Ref<Material> material, Buffer additionalUniforms)
	{
	}

	void RendererHazelLegacy::RenderQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::mat4& transform)
	{
	}

	void RendererHazelLegacy::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material)
	{
	}

	void RendererHazelLegacy::SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material)
	{
	}

	void RendererHazelLegacy::SubmitFullscreenQuadWithOverrides(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<Material> material, Buffer vertexShaderOverrides, Buffer fragmentShaderOverrides)
	{
		s_RendererAPI->SubmitFullscreenQuadWithOverrides(renderCommandBuffer, pipeline, uniformBufferSet, material, vertexShaderOverrides, fragmentShaderOverrides);
	}

	void RendererHazelLegacy::LightCulling(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec2& screenSize, const glm::ivec3& workGroups)
	{
	}

	void RendererHazelLegacy::DispatchComputeShader(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<PipelineCompute> computePipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, const glm::ivec3& workGroups)
	{
	}

	void RendererHazelLegacy::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, const glm::mat4& transform, uint32_t indexCount)
	{
	}

	std::pair<Ref<TextureCubeHazelLegacy>, Ref<TextureCubeHazelLegacy>> RendererHazelLegacy::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	void RendererHazelLegacy::SubmitQuad(Ref<HazelMaterial> material, const glm::mat4& transform)
	{
	}

	void RendererHazelLegacy::SubmitMesh(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform, Ref<HazelMaterialInstance> overrideMaterial)
	{
		// auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterialInstance();
		// auto shader = material->GetShader();
		// TODO: Sort this out
		mesh->GetVertexBuffer()->Bind();
		mesh->GetPipeline()->Bind();
		mesh->GetIndexBuffer()->Bind();

		auto& materials = mesh->GetMaterials();
		for (Ref<SubmeshHazelLegacy> submesh : mesh->GetSubmeshes())
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

			// RendererHazelLegacy::Submit([submesh, material]() {});
			{
				if (material->GetFlag(HazelMaterialFlag::DepthTest))
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
			}
		}
	}

	void RendererHazelLegacy::SubmitMeshWithShader(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform, Ref<HazelShader> shader)
	{
		mesh->GetVertexBuffer()->Bind();
		mesh->GetPipeline()->Bind();
		mesh->GetIndexBuffer()->Bind();

		for (Ref<SubmeshHazelLegacy> submesh : mesh->GetSubmeshes())
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

			// RendererHazelLegacy::Submit([submesh]() {});
			glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
		}
	}

	void RendererHazelLegacy::DrawAABB(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Ref<SubmeshHazelLegacy> submesh : mesh->GetSubmeshes())
		{
			auto& aabb = submesh->BoundingBox;
			auto aabbTransform = transform * submesh->Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void RendererHazelLegacy::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
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

	RenderCommandQueue& RendererHazelLegacy::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}

	void RendererHazelLegacy::SubmitQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Material> material, const glm::mat4& transform)
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

		// RendererHazelLegacy::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
	}

	void RendererHazelLegacy::ClearImage(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<HazelImage2D> image)
	{
		s_RendererAPI->ClearImage(renderCommandBuffer, image);
	}

	Ref<RendererContext> RendererHazelLegacy::GetContext()
	{
		return Application::Get()->GetWindow()->GetRenderContext();
	}

	void RendererHazelLegacy::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRenderer::Shutdown();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

	void RendererHazelLegacy::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void RendererHazelLegacy::EndFrame()
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

	void RendererHazelLegacy::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

#endif

	void RendererHazelLegacy::RenderMesh(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void RendererHazelLegacy::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	Ref<Texture2DHazelLegacy> RendererHazelLegacy::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2DHazelLegacy> RendererHazelLegacy::GetBlackTexture()
	{
		return Ref<Texture2DHazelLegacy>();
	}

	Ref<Texture2DHazelLegacy> RendererHazelLegacy::GetBRDFLutTexture()
	{
		return Ref<Texture2DHazelLegacy>();
	}

	// disabled in some versions of Hazel-dev
	Ref<TextureCubeHazelLegacy> RendererHazelLegacy::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}

	Ref<Environment> RendererHazelLegacy::GetEmptyEnvironment()
	{
		return s_Data->EmptyEnvironment;
	}

	RendererAPI* RendererHazelLegacy::GetRendererAPI()
	{
		return s_RendererAPI;
	}

	RenderCommandQueue& RendererHazelLegacy::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}

	RendererConfigHazelLegacy& RendererHazelLegacy::GetConfig()
	{
		static RendererConfigHazelLegacy config;
		return config;
	}

}
