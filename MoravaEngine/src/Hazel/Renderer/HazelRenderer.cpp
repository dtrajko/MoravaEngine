#include "HazelRenderer.h"

#include "Hazel/Core/Assert.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

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
	};

	static RendererData* s_Data;
	static RenderCommandQueue* s_CommandQueue = nullptr;

	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::OpenGL: return new OpenGLRenderer();
		case RendererAPIType::Vulkan: return new VulkanRenderer();
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

		// Compute shaders
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentMipFilter.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EquirectangularToCubeMap.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/EnvironmentIrradiance.glsl");

		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Grid.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/SceneComposite.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Static.glsl");
		//Renderer::GetShaderLibrary()->Load("assets/shaders/HazelPBR_Anim.glsl");
		//Renderer::GetShaderLibrary()->Load("assets/shaders/Outline.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Skybox.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/Texture.glsl");
		HazelRenderer::GetShaderLibrary()->Load("assets/shaders/ShadowMap.glsl");

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
		// TODO: insert return statement here
	}

	Ref<HazelShaderLibrary>& HazelRenderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
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

	void HazelRenderer::BeginFrame()
	{
	}

	void HazelRenderer::EndFrame()
	{
	}

	void HazelRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
	}

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> HazelRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>>();
	}

	void HazelRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
	}

	void HazelRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
	}

	void HazelRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
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

	void HazelRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
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

	Ref<HazelTexture2D> HazelRenderer::GetWhiteTexture()
	{
		return Ref<HazelTexture2D>();
	}

	Ref<HazelTextureCube> HazelRenderer::GetBlackCubeTexture()
	{
		return Ref<HazelTextureCube>();
	}

	Ref<Environment> HazelRenderer::GetEmptyEnvironment()
	{
		return Ref<Environment>();
	}

	RendererConfig& HazelRenderer::GetConfig()
	{
		// TODO: insert return statement here
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
