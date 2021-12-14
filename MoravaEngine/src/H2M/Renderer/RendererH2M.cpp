/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "RendererH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Platform/OpenGL/OpenGLRendererH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Renderer/PipelineComputeH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Renderer/Renderer2D_LinesH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"
#include "H2M/Renderer/SceneRendererVulkanH2M.h"

#include "Renderer/RendererBasic.h"
#include "Platform/DX11/DX11Renderer.h"


namespace H2M
{

	static std::unordered_map<size_t, RefH2M<PipelineH2M>> s_PipelineCache;

	static RendererAPI_H2M* s_RendererAPI = nullptr;

	struct ShaderDependencies
	{
		std::vector<RefH2M<PipelineComputeH2M>> ComputePipelines;
		std::vector<RefH2M<PipelineH2M>> Pipelines;
		std::vector<RefH2M<MaterialH2M>> Materials;
	};
	static std::unordered_map<size_t, ShaderDependencies> s_ShaderDependencies;

	void RendererH2M::RegisterShaderDependency(RefH2M<ShaderH2M> shader, RefH2M<PipelineH2M> pipeline)
	{
		s_ShaderDependencies[shader->GetHash()].Pipelines.push_back(pipeline);
	}

	void RendererH2M::RegisterShaderDependency(RefH2M<ShaderH2M> shader, RefH2M<MaterialH2M> material)
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
				computePipeline.As<VulkanComputePipelineH2M>()->CreatePipeline();
			}

			for (auto& material : dependencies.Materials)
			{
				material->Invalidate();
			}
		}
	}

	void RendererAPI_H2M::SetAPI(RendererAPITypeH2M api)
	{
		// TODO: make sure this is called at a valid time
		s_CurrentRendererAPI = api;
	}

	struct RendererDataH2M
	{
		RendererConfigH2M Config;

		RefH2M<RenderPassH2M> m_ActiveRenderPass;
		RenderCommandQueueH2M m_CommandQueue;
		RefH2M<ShaderLibraryH2M> m_ShaderLibrary;

		RefH2M<VertexBufferH2M> FullscreenQuadVertexBuffer; // TODO: remove from RendererH2M
		RefH2M<IndexBufferH2M> FullscreenQuadIndexBuffer;   // TODO: remove from RendererH2M
		RefH2M<PipelineH2M> FullscreenQuadPipeline;         // TODO: remove from RendererH2M
		PipelineSpecificationH2M FullscreenQuadPipelineSpec;

		RefH2M<Texture2D_H2M> WhiteTexture;
		RefH2M<Texture2D_H2M> BlackTexture;
		RefH2M<Texture2D_H2M> BRDFLutTexture;
		RefH2M<TextureCubeH2M> BlackCubeTexture;
		RefH2M<EnvironmentH2M> EmptyEnvironment;
	};

	static RendererDataH2M* s_Data = new RendererDataH2M();
	static RenderCommandQueueH2M* s_CommandQueue = nullptr;

	// static std::unordered_map<size_t, RefH2M<Pipeline>> s_PipelineCache;

	static RendererAPI_H2M* InitRendererAPI()
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::OpenGL: return new OpenGLRendererH2M();
			case RendererAPITypeH2M::Vulkan: return new VulkanRendererH2M();
			case RendererAPITypeH2M::DX11:   return new DX11Renderer();
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void RendererH2M::Init()
	{
		s_Data = new RendererDataH2M();
		s_CommandQueue = new RenderCommandQueueH2M();
		s_RendererAPI = InitRendererAPI();

		s_Data->m_ShaderLibrary = RefH2M<ShaderLibraryH2M>::Create();

		//...

		// Compute shaders
		if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
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
		SceneRendererH2M::Init();

		if (RendererAPI_H2M::Current() == RendererAPITypeH2M::Vulkan)
		{
			SceneRendererVulkanH2M::Init();
		}

		std::function<void()> initFunc;

		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::OpenGL:
			initFunc; // = OpenGLRenderer::Init;
			break;
		case RendererAPITypeH2M::Vulkan:
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

		s_Data->FullscreenQuadVertexBuffer = VertexBufferH2M::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
		s_Data->FullscreenQuadIndexBuffer = IndexBufferH2M::Create(indices, 6 * sizeof(uint32_t));

		// ...

		uint32_t whiteTextureData = 0xffffffff;
		s_Data->WhiteTexture = Texture2D_H2M::Create(ImageFormatH2M::RGBA, 1, 1, &whiteTextureData);

		uint32_t blackTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
		s_Data->BlackCubeTexture = TextureCubeH2M::Create(ImageFormatH2M::RGBA, 1, 1, &blackTextureData);

		s_Data->EmptyEnvironment = RefH2M<EnvironmentH2M>::Create(s_Data->BlackCubeTexture, s_Data->BlackCubeTexture);

		//...

	}

	RendererCapabilitiesH2M& RendererH2M::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	RefH2M<ShaderLibraryH2M>& RendererH2M::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	/**** BEGIN to be removed from HazelRenderer ****/
	void RendererH2M::Clear()
	{
		// HazelRenderer::Submit([]() {});
		{
			RendererAPI_H2M::Clear(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void RendererH2M::Clear(float r, float g, float b, float a)
	{
		// HazelRenderer::Submit([=]() {});
		{
			RendererAPI_H2M::Clear(r, g, b, a);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void RendererH2M::ClearMagenta()
	{
		Clear(1, 0, 1);
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void RendererH2M::SetClearColor(float r, float g, float b, float a)
	{
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void RendererH2M::DrawIndexed(uint32_t count, PrimitiveTypeH2M type, bool depthTest)
	{
		// HazelRenderer::Submit([=]() {});
		{
			RendererAPI_H2M::DrawIndexed(count, type, depthTest);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	/**** BEGIN to be removed from HazelRenderer ****/
	void RendererH2M::SetLineThickness(float thickness)
	{
		Log::GetLogger()->warn("HazelRenderer::SetLineThickness({0}): method not implemented!", thickness);

		// HazelRenderer::Submit([=]() {});
		{
			RendererAPI_H2M::SetLineThickness(thickness);
		}
	}
	/**** END to be removed from HazelRenderer ****/

	void RendererH2M::WaitAndRender()
	{
		s_Data->m_CommandQueue.Execute();
	}

	void RendererH2M::BeginRenderPass(RefH2M<RenderPassH2M> renderPass, bool clear)
	{
		H2M_CORE_ASSERT(renderPass, "Render pass cannot be null!");

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
				// RendererH2M::Submit([=]() {});
				{
					RendererAPI::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
				}
			}
		}
		/**** END The obsolete code moved to OpenGLRenderer ****/
	}

	void RendererH2M::EndRenderPass()
	{
		s_RendererAPI->EndRenderPass();

		/**** BEGIN the old version of the method ****

		HZ_CORE_ASSERT(s_Data->m_ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		s_Data->m_ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
		s_Data->m_ActiveRenderPass = nullptr;

		/**** END the old version of the method ****/
	}

	// Used by OpenGLRenderer
	void RendererH2M::SetSceneEnvironment(RefH2M<EnvironmentH2M> environment, RefH2M<Image2D_H2M> shadow)
	{
		s_RendererAPI->SetSceneEnvironment(environment, shadow);
	}

	std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> RendererH2M::CreateEnvironmentMap(const std::string& filepath)
	{
		return s_RendererAPI->CreateEnvironmentMap(filepath);
	}

	void RendererH2M::SubmitQuad(RefH2M<MaterialH2M> material, const glm::mat4& transform)
	{
		bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlagH2M::DepthTest);

			auto shader = material->GetShader();
			shader->SetUniformBuffer("Transform", &transform, sizeof(glm::mat4));
		}

		s_Data->FullscreenQuadVertexBuffer->Bind();
		s_Data->FullscreenQuadPipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();

		DrawIndexed(6, PrimitiveTypeH2M::Triangles, depthTest);
	}

	void RendererH2M::SubmitFullscreenQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material)
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
		s_Data->FullscreenQuadPipeline->Bind();
		s_Data->FullscreenQuadIndexBuffer->Bind();

		RendererH2M::DrawIndexed(6, PrimitiveType::Triangles, depthTest);

		/**** END the old version of the method ****/
	}

	void RendererH2M::SubmitMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<MaterialInstanceH2M> overrideMaterial)
	{
		// auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterialInstance();
		// auto shader = material->GetShader();
		// TODO: Sort this out
		mesh->GetVertexBuffer()->Bind();
		mesh->GetPipeline()->Bind();
		mesh->GetIndexBuffer()->Bind();

		auto& materials = mesh->GetMaterials();
		for (RefH2M<SubmeshH2M> submesh : mesh->GetSubmeshes())
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
				if (material->GetFlag(MaterialFlagH2M::DepthTest))
				{
					glEnable(GL_DEPTH_TEST);
				}
				else
				{
					glDisable(GL_DEPTH_TEST);
				}

				glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
			}
		}
	}

	void RendererH2M::SubmitMeshWithShader(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<ShaderH2M> shader)
	{
		mesh->GetVertexBuffer()->Bind();
		mesh->GetPipeline()->Bind();
		mesh->GetIndexBuffer()->Bind();

		for (RefH2M<SubmeshH2M> submesh : mesh->GetSubmeshes())
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

	void RendererH2M::DrawAABB(RefH2M<MeshH2M> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (RefH2M<SubmeshH2M> submesh : mesh->GetSubmeshes())
		{
			auto& aabb = submesh->BoundingBox;
			auto aabbTransform = transform * submesh->Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void RendererH2M::DrawAABB(const AABB_H2M& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
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
		{
			Renderer2D_LinesH2M::DrawLine(corners[i], corners[(i + 1) % 4], color);
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			Renderer2D_LinesH2M::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			Renderer2D_LinesH2M::DrawLine(corners[i], corners[i + 4], color);
		}
	}

	RenderCommandQueueH2M& RendererH2M::GetRenderCommandQueue()
	{
		return s_Data->m_CommandQueue;
	}

	// ---------------------------------------------------------------

	// Used by OpenGLRenderer
	RefH2M<EnvironmentH2M> RendererH2M::GetEmptyEnvironment()
	{
		// return s_Data->EmptyEnvironment;
		return RefH2M<EnvironmentH2M>();
	}

	uint32_t RendererH2M::GetCurrentFrameIndex()
	{
		// return uint32_t();
		return Application::Get()->GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	RefH2M<RendererContextH2M> RendererH2M::GetContext()
	{
		return Application::Get()->GetWindow()->GetRenderContext();
	}

	void RendererH2M::Shutdown()
	{
		s_ShaderDependencies.clear();
		SceneRendererH2M::Shutdown();
		s_RendererAPI->Shutdown();

		delete s_Data;
		delete s_CommandQueue;
	}

#if 0

	void Renderer::SubmitFullscreenQuad(RefH2M<Material> material)
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

	void RendererH2M::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void RendererH2M::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void RendererH2M::RenderMeshWithoutMaterial(RefH2M<Pipeline> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

#endif

	void RendererH2M::RenderMesh(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform)
	{
		s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void RendererH2M::RenderQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material, const glm::mat4& transform)
	{
		s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	// disabled in some versions of Hazel-dev
	RendererConfigH2M& RendererH2M::GetConfig()
	{
		return s_Data->Config;
	}

	RefH2M<Texture2D_H2M> RendererH2M::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	// disabled in some versions of Hazel-dev
	RefH2M<TextureCubeH2M> RendererH2M::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}

	RendererAPI_H2M* RendererH2M::GetRendererAPI()
	{
		return s_RendererAPI;
	}

}
