#include "OpenGLRenderer.h"

#include "Hazel/Renderer/HazelRenderer.h"

#include <GL/glew.h>

#include "OpenGLMaterial.h"
#include "OpenGLShader.h"
#include "OpenGLTexture.h"
#include "OpenGLImage.h"


namespace Hazel {

	struct OpenGLRendererData
	{
		RenderAPICapabilities RenderCaps;

		Ref<VertexBuffer> m_FullscreenQuadVertexBuffer;
		Ref<IndexBuffer> m_FullscreenQuadIndexBuffer;
		// Ref<Pipeline> m_FullscreenQuadPipeline;
		PipelineSpecification m_FullscreenQuadPipelineSpec;

		Ref<RenderPass> ActiveRenderPass;

		Ref<HazelTexture2D> BRDFLut;
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

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
		{
			if (!depthTest)
				glDisable(GL_DEPTH_TEST);

			GLenum glPrimitiveType = 0;
			switch (type)
			{
			case PrimitiveType::Triangles:
				glPrimitiveType = GL_TRIANGLES;
				break;
			case PrimitiveType::Lines:
				glPrimitiveType = GL_LINES;
				break;
			}

			glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

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
				Log::GetLogger()->error("[OpenGL Debug HIGH] {0}", message);
				// HZ_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				Log::GetLogger()->warn("[OpenGL Debug MEDIUM] {0}", message);
				break;
			case GL_DEBUG_SEVERITY_LOW:
				Log::GetLogger()->info("[OpenGL Debug LOW] {0}", message);
				break;
			case GL_DEBUG_SEVERITY_NOTIFICATION:
				Log::GetLogger()->trace("[OpenGL Debug NOTIFICATION] {0}", message);
				break;
			}
		}
	}

#if 0

	void OpenGLRenderer::Init()
	{
		s_Data = new OpenGLRendererData();
		//	HazelRenderer::Submit([]()
		//	{
		//		glDebugMessageCallback(Utils::OpenGLLogMessage, nullptr);
		//		glEnable(GL_DEBUG_OUTPUT);
		//		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		//	
		//		unsigned int vao;
		//		glGenVertexArrays(1, &vao);
		//		glBindVertexArray(vao);
		//	
		//		glEnable(GL_DEPTH_TEST);
		//		//glEnable(GL_CULL_FACE);
		//		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		//		glFrontFace(GL_CCW);
		//	
		//		glEnable(GL_BLEND);
		//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//	
		//		glEnable(GL_MULTISAMPLE);
		//		glEnable(GL_STENCIL_TEST);
		//	
		//		auto& caps = s_Data->RenderCaps;
		//		caps.Vendor = (const char*)glGetString(GL_VENDOR);
		//		caps.Device = (const char*)glGetString(GL_RENDERER);
		//		caps.Version = (const char*)glGetString(GL_VERSION);
		//	
		//		glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
		//		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);
		//	
		//		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);
		//	
		//		GLenum error = glGetError();
		//		while (error != GL_NO_ERROR)
		//		{
		//			HZ_CORE_ERROR("OpenGL Error {0}", error);
		//			error = glGetError();
		//		}
		//	});

		glDebugMessageCallback(Utils::OpenGLLogMessage, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		
		unsigned int vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glFrontFace(GL_CCW);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_STENCIL_TEST);
		
		auto& caps = s_Data->RenderCaps;
		caps.Vendor = (const char*)glGetString(GL_VENDOR);
		caps.Device = (const char*)glGetString(GL_RENDERER);
		caps.Version = (const char*)glGetString(GL_VERSION);

		glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);

		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);

		GLenum error = glGetError();
		while (error != GL_NO_ERROR)
		{
			// HZ_CORE_ERROR("OpenGL Error {0}", error);
			Log::GetLogger()->error("OpenGL Error {0}", error);
			error = glGetError();
		}

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

		s_Data->m_FullscreenQuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->m_FullscreenQuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		s_Data->BRDFLut = HazelTexture2D::Create("assets/textures/BRDF_LUT.tga");
	}

	void OpenGLRenderer::Shutdown()
	{
		delete s_Data;
	}

	RenderAPICapabilities& OpenGLRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}
#endif

	void OpenGLRenderer::BeginFrame()
	{
	}

	void OpenGLRenderer::EndFrame()
	{
	}

	void OpenGLRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass)
	{
		s_Data->ActiveRenderPass = renderPass;

		renderPass->GetSpecification().TargetFramebuffer->Bind();
		bool clear = true;
		if (clear)
		{
			const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification().ClearColor;
			HazelRenderer::Submit([=]() {
				Utils::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
				});
		}
	}

	void OpenGLRenderer::EndRenderPass()
	{
		s_Data->ActiveRenderPass = nullptr;
	}

	void OpenGLRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
	{
		auto& shader = material->GetShader();

		bool depthTest = true;
		Ref<OpenGLMaterial> glMaterial = material.As<OpenGLMaterial>();
		if (material)
		{
			glMaterial->UpdateForRendering();
			depthTest = material->GetFlag(HazelMaterialFlag::DepthTest);
		}

		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		// HazelRenderer::Submit([depthTest]()
		// {
		// });
		{
			Utils::DrawIndexed(6, PrimitiveType::Triangles, depthTest);
		}
	}

	void OpenGLRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
		if (!environment)
			environment = HazelRenderer::GetEmptyEnvironment();

		// HazelRenderer::Submit([environment, shadow]() mutable
		// {
		// });
		{
			auto shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");
			Ref<OpenGLShader> pbrShader = shader.As<OpenGLShader>();

			if (auto resource = pbrShader->GetShaderResource("u_EnvRadianceTex"))
			{
				Ref<OpenGLTextureCube> radianceMap = environment->RadianceMap.As<OpenGLTextureCube>();
				glBindTextureUnit(resource->GetRegister(), radianceMap->GetRendererID());
			}

			if (auto resource = pbrShader->GetShaderResource("u_EnvIrradianceTex"))
			{
				Ref<OpenGLTextureCube> irradianceMap = environment->IrradianceMap.As<OpenGLTextureCube>();
				glBindTextureUnit(resource->GetRegister(), irradianceMap->GetRendererID());
			}

			if (auto resource = pbrShader->GetShaderResource("u_BRDFLUTTexture"))
			{
				Ref<OpenGLImage2D> brdfLUTImage = s_Data->BRDFLut->GetImage();
				glBindSampler(resource->GetRegister(), brdfLUTImage->GetSamplerRendererID());
				glBindTextureUnit(resource->GetRegister(), brdfLUTImage->GetRendererID());
			}

			if (auto resource = pbrShader->GetShaderResource("u_ShadowMapTexture"))
			{
				Ref<OpenGLImage2D> shadowMapTexture = shadow.As<OpenGLTexture2D>();
				glBindSampler(resource->GetRegister(), shadowMapTexture->GetSamplerRendererID());
				glBindTextureUnit(resource->GetRegister(), shadowMapTexture->GetRendererID());
			}
		}
	}

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> OpenGLRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		const uint32_t cubemapSize = 2048;
		const uint32_t irradianceMapSize = 32;

		Ref<OpenGLTextureCube> envUnfiltered = HazelTextureCube::Create(HazelImageFormat::RGBA32F, cubemapSize, cubemapSize).As<OpenGLTextureCube>();
		Ref<MoravaShader> equirectangularConversionShader = HazelRenderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		Ref<HazelTexture2D> envEquirect = HazelTexture2D::Create(filepath);
		HZ_CORE_ASSERT(envEquirect->GetFormat() == HazelImageFormat::RGBA32F, "Texture is not HDR!");

		equirectangularConversionShader->Bind();
		envEquirect->Bind(1);
		HazelRenderer::Submit([envUnfiltered, cubemapSize, envEquirect]()
			{
				glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
				glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
				glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glGenerateTextureMipmap(envUnfiltered->GetRendererID());
			});

		Ref<MoravaShader> envFilteringShader = HazelRenderer::GetShaderLibrary()->Get("EnvironmentMipFilter");

		Ref<OpenGLTextureCube> envFiltered = HazelTextureCube::Create(HazelImageFormat::RGBA32F, cubemapSize, cubemapSize).As<OpenGLTextureCube>();

		HazelRenderer::Submit([envUnfiltered, envFiltered]()
			{
				glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
					envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
					envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
			});

		envFilteringShader->Bind();
		envUnfiltered->Bind(1);

		HazelRenderer::Submit([envFilteringShader, envUnfiltered, envFiltered, cubemapSize]() {
			const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
			for (int level = 1, size = cubemapSize / 2; level < (int)envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
			{
				glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
				GLint roughnessUniformLocation = glGetUniformLocation(envFilteringShader->GetRendererID(), "u_Uniforms.Roughness");
				HZ_CORE_ASSERT(roughnessUniformLocation != -1);
				glUniform1f(roughnessUniformLocation, (float)level * deltaRoughness);

				const GLuint numGroups = glm::max(1, size / 32);
				glDispatchCompute(numGroups, numGroups, 6);
				glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}
			});

		Ref<MoravaShader> envIrradianceShader = HazelRenderer::GetShaderLibrary()->Get("EnvironmentIrradiance");

		Ref<OpenGLTextureCube> irradianceMap = HazelTextureCube::Create(HazelImageFormat::RGBA32F, irradianceMapSize, irradianceMapSize).As<OpenGLTextureCube>();
		envIrradianceShader->Bind();
		envFiltered->Bind(1);
		HazelRenderer::Submit([irradianceMap]()
			{
				glBindImageTexture(0, irradianceMap->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
				glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
				glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glGenerateTextureMipmap(irradianceMap->GetRendererID());
			});

		return { envFiltered, irradianceMap };
	}

	void OpenGLRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		mesh->m_VertexBuffer->Bind();
		pipeline->Bind();
		mesh->m_IndexBuffer->Bind();

		auto& materials = mesh->GetMaterials();
		for (Submesh& submesh : mesh->m_Submeshes)
		{
			// Material
			auto material = materials[submesh.MaterialIndex].As<OpenGLMaterial>();
			auto shader = material->GetShader();
			material->UpdateForRendering();

			if (false && mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->m_BoneTransforms.size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					mesh->m_MeshShader->SetMat4(uniformName, mesh->m_BoneTransforms[i]);
				}
			}

			auto transformUniform = transform * submesh.Transform;
			shader->SetMat4("u_Renderer.Transform", transformUniform);

			HazelRenderer::Submit([submesh, material]()
				{
					if (material->GetFlag(HazelMaterialFlag::DepthTest))
						glEnable(GL_DEPTH_TEST);
					else
						glDisable(GL_DEPTH_TEST);

					glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
				});
		}
	}

	void OpenGLRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		mesh->m_VertexBuffer->Bind();
		pipeline->Bind();
		mesh->m_IndexBuffer->Bind();

		auto shader = pipeline->GetSpecification().Shader;
		shader->Bind();

		for (Submesh& submesh : mesh->m_Submeshes)
		{
			if (false && mesh->IsAnimated())
			{
				for (size_t i = 0; i < mesh->m_BoneTransforms.size(); i++)
				{
					std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
					mesh->m_MeshShader->SetMat4(uniformName, mesh->m_BoneTransforms[i]);
				}
			}

			auto transformUniform = transform * submesh.Transform;
			shader->SetMat4("u_Renderer.Transform", transformUniform);

			HazelRenderer::Submit([submesh]()
				{
					glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
				});
		}
	}

	void OpenGLRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		s_Data->m_FullscreenQuadVertexBuffer->Bind();
		pipeline->Bind();
		s_Data->m_FullscreenQuadIndexBuffer->Bind();
		Ref<OpenGLMaterial> glMaterial = material.As<OpenGLMaterial>();
		glMaterial->UpdateForRendering();

		auto shader = material->GetShader();
		shader->SetMat4("u_Renderer.Transform", transform);

		HazelRenderer::Submit([material]()
			{
				if (material->GetFlag(HazelMaterialFlag::DepthTest))
					glEnable(GL_DEPTH_TEST);
				else
					glDisable(GL_DEPTH_TEST);

				glDrawElements(GL_TRIANGLES, s_Data->m_FullscreenQuadIndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
			});
	}

}
