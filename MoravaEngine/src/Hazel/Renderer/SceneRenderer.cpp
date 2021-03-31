#include "SceneRenderer.h"

#include "HazelRenderer.h"
#include "Renderer2D.h"
#include "HazelFramebuffer.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>


namespace Hazel {

	struct SceneRendererData
	{
		const HazelScene* ActiveScene = nullptr;
		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			// Resources
			Ref<HazelMaterial> SkyboxMaterial;
			Environment SceneEnvironment;
			HazelLight ActiveLight;
		} SceneData;

		Ref<HazelTexture2D> BRDFLUT;
		Ref<HazelShader> CompositeShader;

		Ref<RenderPass> GeoPass;
		Ref<RenderPass> CompositePass;

		struct DrawCommand
		{
			Ref<HazelMesh> Mesh;
			Ref<HazelMaterial> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;

		// Grid
		Ref<HazelMaterial> GridMaterial;
		Ref<HazelShader> GridShader;
		Ref<HazelMaterial> OutlineMaterial;

		SceneRendererOptions Options;
	};

	static SceneRendererData s_Data;

	void SceneRenderer::Init()
	{
		HazelFramebufferSpecification geoFramebufferSpec;
		geoFramebufferSpec.Width = 1280;
		geoFramebufferSpec.Height = 720;
		geoFramebufferSpec.Format = FramebufferFormat::RGBA16F;
		geoFramebufferSpec.Samples = 8;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification geoRenderPassSpec;
		geoRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(geoFramebufferSpec);
		s_Data.GeoPass = RenderPass::Create(geoRenderPassSpec);

		HazelFramebufferSpecification compFramebufferSpec;
		compFramebufferSpec.Width = 1280;
		compFramebufferSpec.Height = 720;
		compFramebufferSpec.Format = FramebufferFormat::RGBA8;
		compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(compFramebufferSpec);
		s_Data.CompositePass = RenderPass::Create(compRenderPassSpec);

		s_Data.CompositeShader = HazelRenderer::GetShaderLibrary()->Get("SceneComposite");
		s_Data.BRDFLUT = HazelTexture2D::Create("assets/textures/BRDF_LUT.tga");

		// Grid
		s_Data.GridShader = HazelRenderer::GetShaderLibrary()->Get("Grid");
		const float gridScale = 16.025f;
		const float gridSize = 0.025f;
		s_Data.GridShader->Bind();
		s_Data.GridShader->SetUniform("u_Settings.Scale", gridScale);
		s_Data.GridShader->SetUniform("u_Settings.Size", gridSize);

		// Outline
		auto outlineShader = HazelRenderer::GetShaderLibrary()->Get("Outline");
		s_Data.OutlineMaterial = HazelMaterial::Create(outlineShader);
		s_Data.OutlineMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
		s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
	}

	void SceneRenderer::BeginScene(const HazelScene* scene, const SceneRendererCamera& camera)
	{
		HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

		s_Data.ActiveScene = scene;

		s_Data.SceneData.SceneCamera = camera;
		s_Data.SceneData.SkyboxMaterial = scene->m_SkyboxMaterial;
		s_Data.SceneData.SceneEnvironment = scene->m_Environment;
		s_Data.SceneData.ActiveLight = scene->m_Light;
	}

	void SceneRenderer::EndScene()
	{
		HZ_CORE_ASSERT(s_Data.ActiveScene, "");

		s_Data.ActiveScene = nullptr;

		FlushDrawList();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<HazelMaterial> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data.DrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		s_Data.SelectedMeshDrawList.push_back({ mesh, nullptr, transform });
	}

	static Ref<HazelShader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		const uint32_t cubemapSize = 2048;
		const uint32_t irradianceMapSize = 32;

		Ref<HazelTextureCube> envUnfiltered = HazelTextureCube::Create(HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);
		if (!equirectangularConversionShader)
			equirectangularConversionShader = HazelShader::Create("assets/shaders/EquirectangularToCubeMap.glsl");
		Ref<HazelTexture2D> envEquirect = HazelTexture2D::Create(filepath);
		HZ_CORE_ASSERT(envEquirect->GetFormat() == HazelImageFormat::RGBA16F, "Texture is not HDR!");

		equirectangularConversionShader->Bind();
		envEquirect->Bind();
		HazelRenderer::Submit([envUnfiltered, cubemapSize, envEquirect]()
		{
			glBindImageTexture(0, envUnfiltered->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
			glGenerateTextureMipmap(envUnfiltered->GetID());
		});


		if (!envFilteringShader)
			envFilteringShader = HazelShader::Create("assets/shaders/EnvironmentMipFilter.glsl");

		Ref<HazelTextureCube> envFiltered = HazelTextureCube::Create(HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);

		HazelRenderer::Submit([envUnfiltered, envFiltered]()
		{
			glCopyImageSubData(envUnfiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
				envFiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
				envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
		});

		envFilteringShader->Bind();
		envUnfiltered->Bind();

		HazelRenderer::Submit([envUnfiltered, envFiltered, cubemapSize]() {
			const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
			for (int level = 1, size = cubemapSize / 2; level < (int)envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
			{
				const GLuint numGroups = glm::max(1, size / 32);
				glBindImageTexture(0, envFiltered->GetID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
				glProgramUniform1f(envFilteringShader->GetRendererID(), 0, level * deltaRoughness);
				glDispatchCompute(numGroups, numGroups, 6);
			}
		});

		if (!envIrradianceShader)
			envIrradianceShader = HazelShader::Create("assets/shaders/EnvironmentIrradiance.glsl");

		Ref<HazelTextureCube> irradianceMap = HazelTextureCube::Create(HazelImageFormat::RGBA16F, irradianceMapSize, irradianceMapSize);
		envIrradianceShader->Bind();
		envFiltered->Bind();
		HazelRenderer::Submit([irradianceMap]()
		{
			glBindImageTexture(0, irradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
			glGenerateTextureMipmap(irradianceMap->GetID());
		});

		return { envFiltered, irradianceMap };
	}

	void SceneRenderer::GeometryPass()
	{
		bool outline = false && s_Data.SelectedMeshDrawList.size() > 0;

		if (outline)
		{
			HazelRenderer::Submit([]()
			{
			});

			{
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			}
		}

		HazelRenderer::BeginRenderPass(s_Data.GeoPass);

		if (outline)
		{
			HazelRenderer::Submit([]()
			{
				glStencilMask(0);
			});
		}

		auto viewProjection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.ViewMatrix)[3];

		struct EnvironmentUB
		{
			HazelLight lights;
			float Padding = 0.0f;
			glm::vec3 u_CameraPosition;
		};

		EnvironmentUB environmentUB;
		environmentUB.lights = s_Data.SceneData.ActiveLight;
		environmentUB.u_CameraPosition = cameraPosition;
		s_Data.GridShader->SetUniformBuffer("Environment", &environmentUB, sizeof(EnvironmentUB));

		// Skybox
		auto skyboxShader = s_Data.SceneData.SkyboxMaterial->GetShader();
		auto inverseVP = glm::inverse(viewProjection);
		skyboxShader->SetUniformBuffer("Camera", &inverseVP, sizeof(glm::mat4));
		HazelRenderer::SubmitFullscreenQuad(s_Data.SceneData.SkyboxMaterial);

		// Set uniform buffers
		s_Data.GridShader->SetUniformBuffer("Camera", &viewProjection, sizeof(glm::mat4));

#if 0
		// Render entities
		for (auto& dc : s_Data.DrawList)
		{
			auto baseMaterial = dc.Mesh->GetMaterial();
			baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
			baseMaterial->Set("u_CameraPosition", cameraPosition);

			// Environment (TODO: don't do this per mesh)
			baseMaterial->Set("u_EnvRadianceTex", s_Data.SceneData.SceneEnvironment.RadianceMap);
			baseMaterial->Set("u_EnvIrradianceTex", s_Data.SceneData.SceneEnvironment.IrradianceMap);
			baseMaterial->Set("u_BRDFLUTTexture", s_Data.BRDFLUT);

			// Set lights (TODO: move to light environment and don't do per mesh)
			baseMaterial->Set("lights", s_Data.SceneData.ActiveLight);

			auto overrideMaterial = nullptr; // dc.Material;
			HazelRenderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
		}

		if (outline)
		{
			HazelRenderer::Submit([]() {
			});

			{
				glStencilFunc(GL_ALWAYS, 1, 0xff);
				glStencilMask(0xff);
			}
		}

		for (auto& dc : s_Data.SelectedMeshDrawList)
		{
			auto baseMaterial = dc.Mesh->GetMaterial();
			baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
			baseMaterial->Set("u_CameraPosition", cameraPosition);

			// Environment (TODO: don't do this per mesh)
			baseMaterial->Set("u_EnvRadianceTex", s_Data.SceneData.SceneEnvironment.RadianceMap);
			baseMaterial->Set("u_EnvIrradianceTex", s_Data.SceneData.SceneEnvironment.IrradianceMap);
			baseMaterial->Set("u_BRDFLUTTexture", s_Data.BRDFLUT);

			// Set lights (TODO: move to light environment and don't do per mesh)
			baseMaterial->Set("lights", s_Data.SceneData.ActiveLight);

			auto overrideMaterial = nullptr; // dc.Material;
			HazelRenderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
		}
#endif

		if (outline)
		{
			HazelRenderer::Submit([]()
			{
			});

			{
				glStencilFunc(GL_NOTEQUAL, 1, 0xff);
				glStencilMask(0);

				glLineWidth(10);
				glEnable(GL_LINE_SMOOTH);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_DEPTH_TEST);
			}

			// Draw outline here
			s_Data.OutlineMaterial->Set("u_ViewProjection", viewProjection);
			for (auto& dc : s_Data.SelectedMeshDrawList)
			{
				HazelRenderer::SubmitMesh(dc.Mesh, dc.Transform, s_Data.OutlineMaterial);
			}

			HazelRenderer::Submit([]()
			{
				glPointSize(10);
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			});
			for (auto& dc : s_Data.SelectedMeshDrawList)
			{
				HazelRenderer::SubmitMesh(dc.Mesh, dc.Transform, s_Data.OutlineMaterial);
			}

			HazelRenderer::Submit([]()
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glStencilMask(0xff);
				glStencilFunc(GL_ALWAYS, 1, 0xff);
				glEnable(GL_DEPTH_TEST);
			});
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			// s_Data.GridMaterial->Set("u_ViewProjection", viewProjection);

			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
			s_Data.GridShader->Bind();
			s_Data.GridShader->SetUniform("u_VertexUniforms.Transform", transform);
			HazelRenderer::SubmitQuad(nullptr);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
			Renderer2D::BeginScene(viewProjection, true);
			for (auto& dc : s_Data.DrawList)
				HazelRenderer::DrawAABB(dc.Mesh, dc.Transform);
			Renderer2D::EndScene();
		}

		HazelRenderer::EndRenderPass();
	}

	void SceneRenderer::CompositePass()
	{
		HazelRenderer::BeginRenderPass(s_Data.CompositePass);

		float exposure = s_Data.SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples;

		s_Data.CompositeShader->Bind();
		s_Data.CompositeShader->SetUniform("u_Uniforms.Exposure", exposure);
		s_Data.CompositeShader->SetUniform("u_Uniforms.TextureSamples", textureSamples);
		s_Data.GeoPass->GetSpecification().TargetFramebuffer->BindTexture();
		HazelRenderer::SubmitFullscreenQuad(nullptr);
		HazelRenderer::EndRenderPass();
	}

	void SceneRenderer::FlushDrawList()
	{
		HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

		GeometryPass();
		CompositePass();

		s_Data.DrawList.clear();
		s_Data.SelectedMeshDrawList.clear();
		s_Data.SceneData = {};
	}

	Ref<HazelTexture2D> SceneRenderer::GetFinalColorBuffer()
	{
		return s_Data.CompositePass->GetSpecification().TargetFramebuffer;
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return s_Data.CompositePass;
	}

	uint32_t SceneRenderer::GetFinalColorBufferRendererID()
	{
		return s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetColorAttachmentRendererID();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return s_Data.Options;
	}

}
