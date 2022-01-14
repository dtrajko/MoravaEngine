/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLRendererH2M.h"

#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/RenderCommandQueueH2M.h"

#include <GL/glew.h>

#include "OpenGLMaterialH2M.h"
#include "OpenGLShaderH2M.h"
#include "OpenGLTextureH2M.h"
#include "OpenGLImageH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"


namespace H2M
{

	struct OpenGLRendererData
	{
		RendererCapabilitiesH2M RenderCaps;

		RefH2M<VertexBufferH2M> m_FullscreenQuadVertexBuffer;
		RefH2M<IndexBufferH2M> m_FullscreenQuadIndexBuffer;
		RefH2M<PipelineH2M> m_FullscreenQuadPipeline;

		RefH2M<RenderPassH2M> ActiveRenderPass;
		RenderCommandQueueH2M m_CommandQueue;
		RefH2M<ShaderLibraryH2M> m_ShaderLibrary;

		RefH2M<Texture2D_H2M> BRDFLut;
	};

	static OpenGLRendererData* s_Data = nullptr;

	namespace Utils {

		static void Clear(float r, float g, float b, float a)
		{
			glClearColor(r, g, b, a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		}

		static void SetClearColor(float r, float g, float b, float a)
		{
			glClearColor(r, g, b, a);
		}

		static void DrawIndexed(uint32_t count, PrimitiveTypeH2M type, bool depthTest)
		{
			if (!depthTest)
				glDisable(GL_DEPTH_TEST);

			GLenum glPrimitiveTypeH2M = 0;
			switch (type)
			{
			case PrimitiveTypeH2M::Triangles:
				glPrimitiveTypeH2M = GL_TRIANGLES;
				break;
			case PrimitiveTypeH2M::Lines:
				glPrimitiveTypeH2M = GL_LINES;
				break;
			}

			glDrawElements(glPrimitiveTypeH2M, count, GL_UNSIGNED_INT, nullptr);

			if (!depthTest)
				glEnable(GL_DEPTH_TEST);
		}

		static void SetLineThickness(float thickness)
		{
			glLineWidth(thickness);
		}

		static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
		{
			switch (severity)
			{
			case GL_DEBUG_SEVERITY_HIGH:
				Log::GetLogger()->error("[OGLR OpenGL Debug HIGH] {0}", message);
				// HZ_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				Log::GetLogger()->warn("[OGLR OpenGL Debug MEDIUM] {0}", message);
				break;
			case GL_DEBUG_SEVERITY_LOW:
				Log::GetLogger()->info("[OGLR OpenGL Debug LOW] {0}", message);
				break;
			case GL_DEBUG_SEVERITY_NOTIFICATION:
				Log::GetLogger()->trace("[OGLR OpenGL Debug NOTIFICATION] {0}", message);
				break;
			}
		}
	}

	void OpenGLRendererH2M::Init()
	{
		s_Data = new OpenGLRendererData();
		auto& caps = s_Data->RenderCaps;
		caps.Vendor = (const char*)glGetString(GL_VENDOR);
		caps.Device = (const char*)glGetString(GL_RENDERER);
		caps.Version = (const char*)glGetString(GL_VERSION);

		Utils::DumpGPUInfo();

		s_Data->m_ShaderLibrary = RefH2M<ShaderLibraryH2M>::Create();
		// OPENGL ONLY - RendererH2M::Submit([]() { RendererAPI::Init(); });

		SceneRendererH2M::Init();

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

		PipelineSpecificationH2M pipelineSpecification;
		pipelineSpecification.Layout = {
			{ ShaderDataTypeH2M::Float3, "a_Position" },
			{ ShaderDataTypeH2M::Float2, "a_TexCoord" }
		};

		// Missing pipelineSpecification.Shader
		// s_Data->m_FullscreenQuadPipeline = Pipeline::Create(pipelineSpecification);

		s_Data->m_FullscreenQuadVertexBuffer = VertexBufferH2M::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->m_FullscreenQuadIndexBuffer = IndexBufferH2M::Create(indices, 6 * sizeof(uint32_t));

		s_Data->BRDFLut = Texture2D_H2M::Create("assets/textures/BRDF_LUT.tga", false);

		// Renderer2D::Init();
	}

	void OpenGLRendererH2M::Shutdown()
	{
		delete s_Data;
	}

	void OpenGLRendererH2M::BeginFrame()
	{
	}

	void OpenGLRendererH2M::EndFrame()
	{
	}

	void OpenGLRendererH2M::BeginRenderPass(const RefH2M<RenderPassH2M>& renderPass)
	{
		H2M_CORE_ASSERT(renderPass, "Render pass cannot be null!");

		// TODO: Convert all of this into a render command buffer
		s_Data->ActiveRenderPass = renderPass;

		renderPass->GetSpecification().TargetFramebuffer->Bind();

		bool clear = false; // 2nd method parameter in some versions

		if (clear)
		{
			const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification().ClearColor;
			// RendererH2M::Submit([=]() {});
			{
				Utils::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			}
		}
	}

	void OpenGLRendererH2M::EndRenderPass()
	{
		H2M_CORE_ASSERT(s_Data->ActiveRenderPass, "No active render pass! Have you called Renderer::EndRenderPass twice?");
		s_Data->ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
		s_Data->ActiveRenderPass = nullptr;
	}

	void OpenGLRendererH2M::SubmitFullscreenQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material)
	{
		bool depthTest = true;
		if (material)
		{
			material->Bind();
			depthTest = material->GetFlag(MaterialFlagH2M::DepthTest);
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		s_Data->m_FullscreenQuadPipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();

		DrawIndexed(6, PrimitiveTypeH2M::Triangles, depthTest);
	}

	void OpenGLRendererH2M::SetSceneEnvironment(RefH2M<EnvironmentH2M> environment, RefH2M<Image2D_H2M> shadow)
	{
		if (!environment)
		{
			environment = RendererH2M::GetEmptyEnvironment();
		}

		// RendererH2M::Submit([environment, shadow]() mutable {});
		{
			auto shader = RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");
			RefH2M<OpenGLShaderH2M> pbrShader = shader.As<OpenGLShaderH2M>();

			if (auto resource = pbrShader->GetShaderResource("u_EnvRadianceTex"))
			{
				RefH2M<OpenGLTextureCubeH2M> radianceMap = environment->RadianceMap.As<OpenGLTextureCubeH2M>();
				glBindTextureUnit(resource->GetRegister(), radianceMap->GetRendererID());
			}

			if (auto resource = pbrShader->GetShaderResource("u_EnvIrradianceTex"))
			{
				RefH2M<OpenGLTextureCubeH2M> irradianceMap = environment->IrradianceMap.As<OpenGLTextureCubeH2M>();
				glBindTextureUnit(resource->GetRegister(), irradianceMap->GetRendererID());
			}

			if (auto resource = pbrShader->GetShaderResource("u_BRDFLUTTexture"))
			{
				RefH2M<OpenGLImage2D_H2M> brdfLUTImage = s_Data->BRDFLut->GetImage();
				glBindSampler(resource->GetRegister(), brdfLUTImage->GetSamplerRendererID());
				glBindTextureUnit(resource->GetRegister(), brdfLUTImage->GetRendererID());
			}

			if (auto resource = pbrShader->GetShaderResource("u_ShadowMapTexture"))
			{
				RefH2M<OpenGLImage2D_H2M> shadowMapTexture = shadow.As<OpenGLTexture2D_H2M>();
				glBindSampler(resource->GetRegister(), shadowMapTexture->GetSamplerRendererID());
				glBindTextureUnit(resource->GetRegister(), shadowMapTexture->GetRendererID());
			}
		}
	}

	std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> OpenGLRendererH2M::CreateEnvironmentMap(const std::string& filepath)
	{
		Log::GetLogger()->debug("ComputeEnvironmentMaps: {0}", RendererH2M::GetConfig().ComputeEnvironmentMaps);

		if (!RendererH2M::GetConfig().ComputeEnvironmentMaps)
		{
			return { RendererH2M::GetBlackCubeTexture(), RendererH2M::GetBlackCubeTexture() };
		}

		const uint32_t cubemapSize = RendererH2M::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = 32;

		RefH2M<OpenGLTextureCubeH2M> envUnfiltered = TextureCubeH2M::Create(ImageFormatH2M::RGBA32F, cubemapSize, cubemapSize).As<OpenGLTextureCubeH2M>();
		// RefH2M<OpenGLShaderH2M> equirectangularConversionShader = RendererH2M::GetShaderLibrary()->Get("EquirectangularToCubeMap").As<OpenGLShaderH2M>();
		RefH2M<OpenGLShaderH2M> equirectangularConversionShader = ResourceManager::GetShader("Hazel/EquirectangularToCubeMap").As<OpenGLShaderH2M>();
		RefH2M<Texture2D_H2M> envEquirect = Texture2D_H2M::Create(filepath, false);

		// HZ_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");
		if (envEquirect->GetFormat() != ImageFormatH2M::RGBA16F)
		{
			Log::GetLogger()->error("Texture is not HDR!");
		}

		equirectangularConversionShader->Bind();
		envEquirect->Bind(1);
		// RendererH2M::Submit([envUnfiltered, cubemapSize, envEquirect]() {});
		{
			glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glGenerateTextureMipmap(envUnfiltered->GetRendererID());
		}

		// RefH2M<OpenGLShaderH2M> envFilteringShader = RendererH2M::GetShaderLibrary()->Get("EnvironmentMipFilter").As<OpenGLShaderH2M>();
		RefH2M<OpenGLShaderH2M> envFilteringShader = ResourceManager::GetShader("Hazel/EnvironmentMipFilter").As<OpenGLShaderH2M>();

		// s_EnvFiltered = Hazel::TextureCubeH2M::Create(Hazel::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize, true);
		RefH2M<OpenGLTextureCubeH2M> envFiltered = TextureCubeH2M::Create(ImageFormatH2M::RGBA32F, cubemapSize, cubemapSize).As<OpenGLTextureCubeH2M>();

		// RendererH2M::Submit([envUnfiltered, envFiltered]() {});
		{
			glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
				envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
				envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
		}

		envFilteringShader->Bind();
		envUnfiltered->Bind(1);

		// RendererH2M::Submit([envFilteringShader, envUnfiltered, envFiltered, cubemapSize]() {});
		{
			const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
			for (uint32_t level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
			{
				Log::GetLogger()->debug("BEGIN EnvFiltering size {0} level {1}/{2}", size, level, envFiltered->GetMipLevelCount());

				glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

				const GLint roughnessUniformLocation = glGetUniformLocation(envFilteringShader->GetRendererID(), "u_Uniforms.Roughness");
				H2M_CORE_ASSERT(roughnessUniformLocation != -1);
				glUniform1f(roughnessUniformLocation, (float)level * deltaRoughness);

				// glProgramUniform1f(envFilteringShader->GetRendererID(), 0, level * deltaRoughness);

				const GLuint numGroups = glm::max(1u, size / 32);
				glDispatchCompute(numGroups, numGroups, 6);
				glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, envFiltered->GetMipLevelCount());
			}
		}

		// RefH2M<OpenGLShaderH2M> envIrradianceShader = RendererH2M::GetShaderLibrary()->Get("EnvironmentIrradiance").As<OpenGLShaderH2M>();
		RefH2M<OpenGLShaderH2M> envIrradianceShader = ResourceManager::GetShader("Hazel/EnvironmentIrradiance").As<OpenGLShaderH2M>();

		// s_IrradianceMap = Hazel::TextureCubeH2M::Create(Hazel::ImageFormatH2M::RGBA16F, irradianceMapSize, irradianceMapSize, true);
		RefH2M<OpenGLTextureCubeH2M> irradianceMap = TextureCubeH2M::Create(ImageFormatH2M::RGBA32F, irradianceMapSize, irradianceMapSize).As<OpenGLTextureCubeH2M>();

		envIrradianceShader->Bind();
		envFiltered->Bind(1);

		// RendererH2M::Submit([irradianceMap, envIrradianceShader]() {});
		{
			glBindImageTexture(0, irradianceMap->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

			const GLint samplesUniformLocation = glGetUniformLocation(envIrradianceShader->GetRendererID(), "u_Uniforms.Samples");
			H2M_CORE_ASSERT(samplesUniformLocation != -1);
			const uint32_t samples = RendererH2M::GetConfig().IrradianceMapComputeSamples;
			glUniform1ui(samplesUniformLocation, samples);

			glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glGenerateTextureMipmap(irradianceMap->GetRendererID());
		}

		return { envFiltered, irradianceMap };
	}

	void OpenGLRendererH2M::RenderMesh(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform)
	{
		mesh->GetVertexBuffer()->Bind();
		pipeline->Bind();
		mesh->GetIndexBuffer()->Bind();

		auto& materials = mesh->GetMaterials();
		for (RefH2M<SubmeshH2M> submesh : mesh->GetSubmeshes())
		{
			// Material
			auto material = materials[submesh->MaterialIndex].As<OpenGLMaterialH2M>();
			auto shader = material->GetShader();
			material->UpdateForRendering();

			if (false && mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->GetBoneTransforms().size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					mesh->GetMeshShader()->SetMat4(uniformName, mesh->GetBoneTransforms()[i]);
				}
			}

			auto transformUniform = transform * submesh->Transform;
			shader->SetMat4("u_Renderer.Transform", transformUniform);

			RendererH2M::Submit([submesh, material]()
			{
				if (material->GetFlag(MaterialFlagH2M::DepthTest))
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
			});
		}
	}

	void OpenGLRendererH2M::RenderMeshWithoutMaterial(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform)
	{
		mesh->GetVertexBuffer()->Bind();
		pipeline->Bind();
		mesh->GetIndexBuffer()->Bind();

		auto shader = pipeline->GetSpecification().Shader;
		shader->Bind();

		for (RefH2M<SubmeshH2M> submesh : mesh->GetSubmeshes())
		{
			if (false && mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->GetBoneTransforms().size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					mesh->GetMeshShader()->SetMat4(uniformName, mesh->GetBoneTransforms()[i]);
				}
			}

			auto transformUniform = transform * submesh->Transform;
			shader->SetMat4("u_Renderer.Transform", transformUniform);

			RendererH2M::Submit([submesh]()
			{
				glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);
			});
		}
	}

	void OpenGLRendererH2M::RenderQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material, const glm::mat4& transform)
	{
		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();

		RefH2M<OpenGLMaterialH2M> glMaterial = material.As<OpenGLMaterialH2M>();
		glMaterial->UpdateForRendering();

		auto shader = material->GetShader();
		shader->SetMat4("u_Renderer.Transform", transform);

		// RendererH2M::Submit([material]() {});
		{
			if (material->GetFlag(MaterialFlagH2M::DepthTest))
			{
				glEnable(GL_DEPTH_TEST);
			}
			else
			{
				glDisable(GL_DEPTH_TEST);
			}

			glDrawElements(GL_TRIANGLES, s_Data->m_FullscreenQuadIndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
		}
	}

	void OpenGLRendererH2M::DrawIndexed(uint32_t indexCount, PrimitiveTypeH2M type, bool depthTest)
	{
		if (!depthTest)
		{
			glDisable(GL_DEPTH_TEST);
		}

		GLenum glPrimitiveType = 0;
		switch (type)
		{
		case PrimitiveTypeH2M::Triangles:
			glPrimitiveType = GL_TRIANGLES;
			break;
		case PrimitiveTypeH2M::Lines:
			glPrimitiveType = GL_LINES;
			break;
		}

		glDrawElements(glPrimitiveType, indexCount, GL_UNSIGNED_INT, nullptr);

		if (!depthTest)
		{
			glEnable(GL_DEPTH_TEST);
		}
	}

	void OpenGLRendererH2M::DrawLines(RefH2M<VertexArrayH2M> vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererH2M::SetLineWidth(float width)
	{
	}

	RendererCapabilitiesH2M& OpenGLRendererH2M::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

}
