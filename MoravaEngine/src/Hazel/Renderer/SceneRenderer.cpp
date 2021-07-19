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
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			HazelLight ActiveLight;
		} SceneData;

		Ref<HazelTexture2D> BRDFLUT;
		Ref<HazelShader> CompositeShader;
		Ref<MoravaShader> BloomBlurShader;
		Ref<MoravaShader> BloomBlendShader;

		Ref<RenderPass> GeoPass;
		Ref<RenderPass> CompositePass;
		Ref<RenderPass> BloomBlurPass[2];
		Ref<RenderPass> BloomBlendPass;

		Ref<MoravaShader> ShadowMapShader, ShadowMapAnimShader;
		Ref<RenderPass> ShadowMapRenderPass[4];
		float ShadowMapSize = 20.0f;
		float LightDistance = 0.1f;
		glm::mat4 LightMatrices[4];
		glm::mat4 LightViewMatrix;
		float CascadeSplitLambda = 0.91f;
		glm::vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 15.0f, CascadeNearPlaneOffset = -15.0f;
		bool ShowCascades = false;
		bool SoftShadows = true;
		float LightSize = 0.25f;
		float MaxShadowDistance = 200.0f;
		float ShadowFade = 25.0f;
		float CascadeTransitionFade = 1.0f;
		bool CascadeFading = true;

		bool EnableBloom = false;
		float BloomThreshold = 1.5f;

		glm::vec2 FocusPoint = { 0.5f, 0.5f };

		RendererID ShadowMapSampler;
		Ref<Material> CompositeMaterial;

		Ref<Pipeline> GeometryPipeline;
		Ref<Pipeline> CompositePipeline;
		Ref<Pipeline> ShadowPassPipeline;
		Ref<Pipeline> SkyboxPipeline;
		Ref<Material> SkyboxMaterial;

		struct DrawCommand
		{
			Ref<HazelMesh> Mesh;
			Ref<HazelMaterial> Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		// Grid
		Ref<HazelMaterial> GridMaterial;
		Ref<HazelShader> GridShader;
		Ref<HazelMaterial> OutlineMaterial;
		Ref<HazelMaterial> OutlineAnimMaterial;

		SceneRendererOptions Options;
	};

	struct SceneRendererStats
	{
		float ShadowPass = 0.0f;
		float GeometryPass = 0.0f;
		float CompositePass = 0.0f;

		Timer ShadowPassTimer;
		Timer GeometryPassTimer;
		Timer CompositePassTimer;
	};

	static SceneRendererData* s_Data;
	static SceneRendererStats s_Stats;


	void SceneRenderer::Init()
	{
		s_Data = new SceneRendererData();

#if 0
		HazelFramebufferSpecification geoFramebufferSpec;
		geoFramebufferSpec.Width = 1280;
		geoFramebufferSpec.Height = 720;
		geoFramebufferSpec.Format = FramebufferFormat::RGBA16F;
		geoFramebufferSpec.Samples = 8;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification geoRenderPassSpec;
		geoRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(geoFramebufferSpec);
		s_Data->GeoPass = RenderPass::Create(geoRenderPassSpec);

		HazelFramebufferSpecification compFramebufferSpec;
		compFramebufferSpec.Width = 1280;
		compFramebufferSpec.Height = 720;
		compFramebufferSpec.Format = FramebufferFormat::RGBA8;
		compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(compFramebufferSpec);
		s_Data->CompositePass = RenderPass::Create(compRenderPassSpec);

		s_Data->CompositeShader = HazelRenderer::GetShaderLibrary()->Get("SceneComposite");
		s_Data->BRDFLUT = HazelTexture2D::Create("assets/textures/BRDF_LUT.tga");

		// Grid
		s_Data->GridShader = HazelRenderer::GetShaderLibrary()->Get("Grid");
		const float gridScale = 16.025f;
		const float gridSize = 0.025f;
		s_Data->GridShader->Bind();
		s_Data->GridShader->SetUniform("u_Settings.Scale", gridScale);
		s_Data->GridShader->SetUniform("u_Settings.Size", gridSize);

		// Outline
		auto outlineShader = HazelRenderer::GetShaderLibrary()->Get("Outline");
		s_Data->OutlineMaterial = HazelMaterial::Create(outlineShader);
		s_Data->OutlineMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
#endif

#if 0
		s_Data->CompositeShader = MoravaShader::Create("assets/shaders/SceneComposite.glsl");
		s_Data->BloomBlurShader = MoravaShader::Create("assets/shaders/BloomBlur.glsl");
		s_Data->BloomBlendShader = MoravaShader::Create("assets/shaders/BloomBlend.glsl");
#endif

	}

	void SceneRenderer::Shutdown()
	{
		delete s_Data;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		s_Data->GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
		s_Data->CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
	}

	void SceneRenderer::BeginScene(const HazelScene* scene, const SceneRendererCamera& camera)
	{
		HZ_CORE_ASSERT(!s_Data->ActiveScene, "");

		s_Data->ActiveScene = scene;

		s_Data->SceneData.SceneCamera = camera;
		s_Data->SceneData.SkyboxMaterial = scene->m_SkyboxMaterial;
		s_Data->SceneData.SceneEnvironment = scene->m_Environment;
		s_Data->SceneData.ActiveLight = scene->m_Light;
	}

	void SceneRenderer::EndScene()
	{
		HZ_CORE_ASSERT(s_Data->ActiveScene, "");

		s_Data->ActiveScene = nullptr;

		FlushDrawList();
	}

	void SceneRenderer::SubmitMesh(MeshComponent meshComponent, TransformComponent transformComponent)
	{
		SubmitMesh(meshComponent.Mesh, transformComponent.GetTransform(), Ref<Material>());
	}

	void SceneRenderer::SubmitSelectedMesh(MeshComponent meshComponent, TransformComponent transformComponent)
	{
		SubmitSelectedMesh(meshComponent.Mesh, transformComponent.GetTransform());
	}

	void SceneRenderer::SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<Material> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data->DrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_Data->SelectedMeshDrawList.push_back({ mesh, Ref<HazelMaterial>(), transform });
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

		if (!envFilteringShader) {
			envFilteringShader = HazelShader::Create("assets/shaders/EnvironmentMipFilter.glsl");
		}

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

		if (!envIrradianceShader) {
			envIrradianceShader = HazelShader::Create("assets/shaders/EnvironmentIrradiance.glsl");
		}

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
		bool outline = false && s_Data->SelectedMeshDrawList.size() > 0;

		if (outline)
		{
			HazelRenderer::Submit([]()
			{
			});

			{
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			}
		}

		HazelRenderer::BeginRenderPass(s_Data->GeoPass);

		if (outline)
		{
			HazelRenderer::Submit([]()
			{
				glStencilMask(0);
			});
		}

		auto viewProjection = s_Data->SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data->SceneData.SceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(s_Data->SceneData.SceneCamera.ViewMatrix)[3];

		struct EnvironmentUB
		{
			HazelLight lights;
			float Padding = 0.0f;
			glm::vec3 u_CameraPosition;
		};

		EnvironmentUB environmentUB;
		environmentUB.lights = s_Data->SceneData.ActiveLight;
		environmentUB.u_CameraPosition = cameraPosition;
		s_Data->GridShader->SetUniformBuffer("Environment", &environmentUB, sizeof(EnvironmentUB));

		// Skybox
		auto skyboxShader = s_Data->SceneData.SkyboxMaterial->GetShader();
		auto inverseVP = glm::inverse(viewProjection);
		skyboxShader->SetUniformBuffer("Camera", &inverseVP, sizeof(glm::mat4));
		HazelRenderer::SubmitFullscreenQuad(/*s_Data->SkyboxPipeline,*/s_Data->SceneData.SkyboxMaterial);

		// Set uniform buffers
		s_Data->GridShader->SetUniformBuffer("Camera", &viewProjection, sizeof(glm::mat4));

#if 0
		// Render entities
		for (auto& dc : s_Data->DrawList)
		{
			auto baseMaterial = dc.Mesh->GetMaterial();
			baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
			baseMaterial->Set("u_CameraPosition", cameraPosition);

			// Environment (TODO: don't do this per mesh)
			baseMaterial->Set("u_EnvRadianceTex", s_Data->SceneData.SceneEnvironment.RadianceMap);
			baseMaterial->Set("u_EnvIrradianceTex", s_Data->SceneData.SceneEnvironment.IrradianceMap);
			baseMaterial->Set("u_BRDFLUTTexture", s_Data->BRDFLUT);

			// Set lights (TODO: move to light environment and don't do per mesh)
			baseMaterial->Set("lights", s_Data->SceneData.ActiveLight);

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

		for (auto& dc : s_Data->SelectedMeshDrawList)
		{
			auto baseMaterial = dc.Mesh->GetMaterial();
			baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
			baseMaterial->Set("u_CameraPosition", cameraPosition);

			// Environment (TODO: don't do this per mesh)
			baseMaterial->Set("u_EnvRadianceTex", s_Data->SceneData.SceneEnvironment.RadianceMap);
			baseMaterial->Set("u_EnvIrradianceTex", s_Data->SceneData.SceneEnvironment.IrradianceMap);
			baseMaterial->Set("u_BRDFLUTTexture", s_Data->BRDFLUT);

			// Set lights (TODO: move to light environment and don't do per mesh)
			baseMaterial->Set("lights", s_Data->SceneData.ActiveLight);

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
			s_Data->OutlineMaterial->Set("u_ViewProjection", viewProjection);
			for (auto& dc : s_Data->SelectedMeshDrawList)
			{
				HazelRenderer::SubmitMesh(dc.Mesh, dc.Transform, s_Data->OutlineMaterial);
			}

			HazelRenderer::Submit([]()
			{
				glPointSize(10);
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			});
			for (auto& dc : s_Data->SelectedMeshDrawList)
			{
				HazelRenderer::SubmitMesh(dc.Mesh, dc.Transform, s_Data->OutlineMaterial);
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
			// s_Data->GridMaterial->Set("u_ViewProjection", viewProjection);

			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
			s_Data->GridShader->Bind();
			s_Data->GridShader->SetUniform("u_VertexUniforms.Transform", transform);
			HazelRenderer::SubmitQuad(Ref<HazelMaterial>());
		}

		if (GetOptions().ShowBoundingBoxes)
		{
			Renderer2D::BeginScene(viewProjection, true);
			for (auto& dc : s_Data->DrawList)
				HazelRenderer::DrawAABB(dc.Mesh, dc.Transform);
			Renderer2D::EndScene();
		}

		HazelRenderer::EndRenderPass();
	}

	void SceneRenderer::FlushDrawList()
	{
		HZ_CORE_ASSERT(!s_Data->ActiveScene, "");

		GeometryPass();
		CompositePass();

		s_Data->DrawList.clear();
		s_Data->SelectedMeshDrawList.clear();
		s_Data->SceneData = {};
	}

	Ref<HazelTexture2D> SceneRenderer::GetFinalColorBuffer()
	{
		return s_Data->CompositePass->GetSpecification().TargetFramebuffer;
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return s_Data->CompositePass;
	}

	uint32_t SceneRenderer::GetFinalColorBufferRendererID()
	{
		return s_Data->CompositePass->GetSpecification().TargetFramebuffer->GetImage();
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return s_Data->Options;
	}

	void SceneRenderer::OnImGuiRender()
	{
	}

	void SceneRenderer::CompositePass()
	{
		HazelRenderer::BeginRenderPass(s_Data->CompositePass);

		float exposure = s_Data->SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = s_Data->GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples;

		s_Data->CompositeShader->Bind();
		s_Data->CompositeShader->SetUniform("u_Uniforms.Exposure", exposure);
		s_Data->CompositeShader->SetUniform("u_Uniforms.TextureSamples", textureSamples);
		s_Data->GeoPass->GetSpecification().TargetFramebuffer->BindTexture();
		HazelRenderer::SubmitFullscreenQuad(/*s_Data->CompositePipeline,*/s_Data->CompositeMaterial);
		HazelRenderer::EndRenderPass();
	}

	void SceneRenderer::BloomBlurPass()
	{
	}

	struct FrustumBounds
	{
		float r, l, b, t, f, n;
	};

	struct CascadeData
	{
		glm::mat4 ViewProj;
		glm::mat4 View;
		float SplitDepth;
	};

	static void CalculateCascades(CascadeData* cascades, const glm::vec3& lightDirection)
	{
		FrustumBounds frustumBounds[3];

		auto& sceneCamera = s_Data->SceneData.SceneCamera;
		auto viewProjection = sceneCamera.Camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		// TODO: less hard-coding!
		float nearClip = 0.1f;
		float farClip = 1000.0f;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in
		// https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus

		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = s_Data->CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Manually set cascades here
		// cascadeSplits[0] = 0.05f;
		// cascadeSplits[1] = 0.15f;
		// cascadeSplits[2] = 0.3f;
		// cascadeSplits[3] = 1.0f;

// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + s_Data->CascadeNearPlaneOffset, maxExtents.z - minExtents.z + s_Data->CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			const float ShadowMapResolution = 4096.0f;
			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::ShadowMapPass()
	{
		auto& directionalLights = s_Data->SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows) { return; }

		CascadeData cascades[4];
		CalculateCascades(cascades, directionalLights[0].Direction);
		s_Data->LightViewMatrix = cascades[0].View;

		// HazelRenderer::Submit([](){});
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}

		for (int i = 0; i < 4; i++)
		{
			s_Data->CascadeSplits[i] = cascades[i].SplitDepth;

			HazelRenderer::BeginRenderPass(s_Data->ShadowMapRenderPass[i]);

			glm::mat4 shadowMapVP = cascades[i].ViewProj;

			static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
			s_Data->LightMatrices[i] = scaleBiasMatrix * cascades[i].ViewProj;


			// Render entities
			for (auto& dc : s_Data->ShadowPassDrawList)
			{
				Ref<HazelShader> shader = dc.Mesh->IsAnimated() ? s_Data->ShadowMapAnimShader : s_Data->ShadowMapShader;
				shader->SetMat4("u_ViewProjection", shadowMapVP);
				HazelRenderer::SubmitMeshWithShader(dc.Mesh, dc.Transform, shader);
			}

			HazelRenderer::EndRenderPass();
		}
	}

}
