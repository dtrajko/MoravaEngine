#include "SceneRendererVulkan.h"

#include "Hazel/Renderer/HazelFramebuffer.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/Renderer2D.h"

#include "HazelLegacy/Platform/Vulkan/VulkanFramebufferHazelLegacy.h"
#include "HazelLegacy/Platform/Vulkan/VulkanRendererHazelLegacy.h"
#include "HazelLegacy/Renderer/RendererHazelLegacy.h"
#include "HazelLegacy/Renderer/SceneRendererHazelLegacy.h"


namespace Hazel
{
	static std::vector<std::thread> s_ThreadPool;

	struct SceneRendererData
	{
		const HazelScene* ActiveScene = nullptr;
		struct SceneInfo
		{
			SceneRendererCameraVulkan SceneCamera;

			// Resources
			Ref<HazelMaterial> SkyboxMaterial;
			Environment SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			HazelDirLight ActiveLight;
			glm::vec3 LightDirectionTemp;
		} SceneData;

		Ref<HazelTexture2D> BRDFLUT;
		Ref<HazelShader> CompositeShader;
		Ref<HazelMaterial> CompositeMaterial;
		Ref<MoravaShader> BloomBlurShader;
		Ref<MoravaShader> BloomBlendShader;

		Ref<RenderPass> GeoPass;
		Ref<RenderPass> CompositePass;
		Ref<RenderPass> BloomBlurPass[2];
		Ref<RenderPass> BloomBlendPass;

		Ref<Pipeline> GeometryPipeline;
		Ref<Pipeline> CompositePipeline;
		Ref<Pipeline> SkyboxPipeline;
		Ref<Pipeline> ShadowPassPipeline;
		Ref<HazelMaterial> SkyboxMaterial;

		struct DrawCommand
		{
			Ref<HazelMesh> Mesh;
			Ref<HazelMaterial> Material;
			glm::mat4 Transform;
		};

		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ColliderDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		// Grid
		Ref<Pipeline> GridPipeline;
		Ref<HazelShader> GridShader;
		Ref<HazelMaterial> GridMaterial;
		Ref<HazelMaterial> OutlineMaterial;
		Ref<HazelMaterial> OutlineAnimMaterial;

		SceneRendererOptionsVulkan Options;

		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		bool NeedsResize = false;

		VkDescriptorImageInfo ColorBufferInfo;
	};

	static SceneRendererData s_Data;

	SceneRendererVulkan::SceneRendererVulkan(Ref<HazelScene> scene, SceneRendererSpecificationVulkan specification)
		: m_Scene(scene), m_Specification(specification)
	{
		Init();
	}

	void SceneRendererVulkan::Init()
	{
		HazelFramebufferSpecification geoFramebufferSpec = {};
		geoFramebufferSpec.Width = 1280;
		geoFramebufferSpec.Height = 720;
		// geoFramebufferSpec.Format = FramebufferFormat::RGBA16F;
		geoFramebufferSpec.Samples = 8;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification geoRenderPassSpec = {};
		geoRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(geoFramebufferSpec);
		/****
		s_Data.GeoPass = RenderPass::Create(geoRenderPassSpec);

		HazelFramebufferSpecification compFramebufferSpec = {};
		compFramebufferSpec.Width = 1280;
		compFramebufferSpec.Height = 720;
		compFramebufferSpec.Format = FramebufferFormat::RGBA8;
		compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(compFramebufferSpec);
		s_Data.CompositePass = RenderPass::Create(compRenderPassSpec);

		s_Data.CompositeShader = HazelRenderer::GetShaderLibrary()->Get("SceneComposite");
		s_Data.CompositeMaterial = HazelMaterial::Create(s_Data.CompositeShader, "CompositeMaterial");
		s_Data.BRDFLUT = HazelTexture2D::Create("assets/textures/BRDF_LUT.tga");

		// Grid pipeline
		{
			s_Data.GridShader = HazelRenderer::GetShaderLibrary()->Get("Grid");
			const float gridScale = 16.025f;
			const float gridSize = 0.025f;
			s_Data.GridMaterial = HazelMaterial::Create(s_Data.GridShader);
			// s_Data.GridMaterial->Set("u_Settings.Scale", gridScale); // TODO: fix "We currently only support ONE material buffer!"
			// s_Data.GridMaterial->Set("u_Settings.Size", gridSize);   // TODO: fix "We currently only support ONE material buffer!"

			PipelineSpecification pipelineSpec = {};
			pipelineSpec.DebugName = "Grid";
			pipelineSpec.Shader = s_Data.GridShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpec.RenderPass = s_Data.GeoPass;
			s_Data.GridPipeline = Pipeline::Create(pipelineSpec); // fragment shader writes to output location 1 with no matching attachment
		}

		// Outline
		auto outlineShader = HazelRenderer::GetShaderLibrary()->Get("Outline");
		s_Data.OutlineMaterial = HazelMaterial::Create(outlineShader);
		s_Data.OutlineMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);

		// Skybox pipeline
		{
			auto skyboxShader = HazelRenderer::GetShaderLibrary()->Get("Skybox");

			PipelineSpecification pipelineSpec = {};
			pipelineSpec.DebugName = "Skybox";
			pipelineSpec.Shader = skyboxShader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpec.RenderPass = s_Data.GeoPass;
			// s_Data.SkyboxPipeline = Pipeline::Create(pipelineSpec);

			s_Data.SkyboxMaterial = HazelMaterial::Create(skyboxShader);
			s_Data.SkyboxMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);
		}

		// Geometry pipeline
		{
			HazelFramebufferSpecification spec = {};
			Ref<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);

			PipelineSpecification pipelineSpecification = {};
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec = {};
			renderPassSpec.TargetFramebuffer = framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			// s_Data.GeometryPipeline = Pipeline::Create(pipelineSpecification); // fragment shader writes to output location 1 with no matching attachment
		}

		// Composite pipeline
		{
			HazelFramebufferSpecification spec = {};;
			Ref<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);
			framebuffer->AddResizeCallback([](Ref<HazelFramebuffer> framebuffer)
			{
				// HazelRenderer::Submit([framebuffer]() mutable {});
				{
					auto vulkanFB = framebuffer.As<VulkanFramebuffer>();
					s_Data.ColorBufferInfo = vulkanFB->GetVulkanDescriptorInfo();
				}
			});

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = HazelRenderer::GetShaderLibrary()->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			// s_Data.CompositePipeline = Pipeline::Create(pipelineSpecification); // fragment shader writes to output location 1 with no matching attachment
		}
		****/
	}

	void SceneRendererVulkan::SetScene(Ref<HazelScene> scene)
	{
	}

	void SceneRendererVulkan::Shutdown()
	{
	}

	void SceneRendererVulkan::SetViewportSize(uint32_t width, uint32_t height)
	{
		s_Data.ViewportWidth = width;
		s_Data.ViewportHeight = height;
		s_Data.NeedsResize = true;
	}

	void SceneRendererVulkan::BeginScene(const HazelScene* scene, const SceneRendererCameraVulkan& camera)
	{
		HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

		s_Data.ActiveScene = scene;

		s_Data.SceneData.SceneCamera = camera;
		// s_Data.SceneData.SkyboxMaterial = scene->GetSkyboxMaterial();
		s_Data.SceneData.SceneEnvironment = scene->GetEnvironment();
		s_Data.SceneData.SkyboxLod = scene->GetSkyboxLod();
		s_Data.SceneData.ActiveLight = scene->GetLight();

		if (s_Data.NeedsResize)
		{
			s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(s_Data.ViewportWidth, s_Data.ViewportHeight);
			s_Data.CompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(s_Data.ViewportWidth, s_Data.ViewportHeight);
			s_Data.NeedsResize = false;
		}

		RendererHazelLegacy::SetSceneEnvironment(Ref<SceneRendererHazelLegacy>(), &s_Data.SceneData.SceneEnvironment, Ref<HazelImage2D>(), Ref<HazelImage2D>());
	}

	void SceneRendererVulkan::EndScene()
	{
		HZ_CORE_ASSERT(s_Data.ActiveScene, "");

		s_Data.ActiveScene = nullptr;

		FlushDrawList();
#if MULTI_THREAD
		// Ref<SceneRendererVulkan> instance = this;
		// s_ThreadPool.emplace_back(([instance]() mutable {}))
		{
			// FlushDrawList();
		}
#else
		// FlushDrawList(); // TODO: implement the method
#endif

		// m_Active = false;
	}

	void SceneRendererVulkan::UpdateHBAOData()
	{
	}

	void SceneRendererVulkan::WaitForThreads()
	{
		for (uint32_t i = 0; i < s_ThreadPool.size(); i++)
		{
			s_ThreadPool[i].join();
		}

		s_ThreadPool.clear();
	}

	void SceneRendererVulkan::SubmitMesh(MeshComponentHazelLegacy meshComponent, TransformComponent transformComponent)
	{
		SubmitMesh(meshComponent.Mesh, transformComponent.GetTransform(), Ref<HazelMaterial>());
	}

	void SceneRendererVulkan::SubmitSelectedMesh(MeshComponentHazelLegacy meshComponent, TransformComponent transformComponent)
	{
		SubmitSelectedMesh(meshComponent.Mesh, transformComponent.GetTransform());
	}

	void SceneRendererVulkan::SubmitMesh(Ref<HazelMesh> mesh, const glm::mat4& transform, Ref<HazelMaterial> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data.DrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRendererVulkan::SubmitSelectedMesh(Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_Data.SelectedMeshDrawList.push_back({ mesh, Ref<HazelMaterial>(), transform });
		// s_Data.ShadowPassDrawList.push_back({ mesh, Ref<HazelMaterial>, transform });
	}

	Ref<RenderPass> SceneRendererVulkan::GetFinalRenderPass()
	{
		return Ref<RenderPass>();
	}

	Ref<HazelTexture2D> SceneRendererVulkan::GetFinalPassImage()
	{
		return Ref<HazelTexture2D>();
	}

	void SceneRendererVulkan::GeometryPass()
	{
		RendererHazelLegacy::BeginRenderPass(Ref<RenderCommandBuffer>(), s_Data.GeoPass);

		auto viewProjection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.ViewMatrix)[3];
		// glm::vec3 cameraPosition = s_Data.SceneData.SceneCamera.Camera.GetPosition();

		// HazelRenderer::Submit([viewProjection, cameraPosition]() {});
		{
			auto inverseVP = glm::inverse(viewProjection);
			auto shader = s_Data.GridMaterial->GetShader().As<VulkanShader>();
			void* ubPtr = shader->MapUniformBuffer(0);
			struct ViewProj
			{
				glm::mat4 ViewProjection;
				glm::mat4 InverseViewProjection;
			};
			ViewProj viewProj;
			viewProj.ViewProjection = viewProjection;
			viewProj.InverseViewProjection = inverseVP;
			memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			shader->UnmapUniformBuffer(0);

			shader = s_Data.SkyboxMaterial->GetShader().As<VulkanShader>();
			ubPtr = shader->MapUniformBuffer(0);
			memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			shader->UnmapUniformBuffer(0);

			shader = s_Data.GeometryPipeline->GetSpecification().Shader.As<VulkanShader>();
			ubPtr = shader->MapUniformBuffer(0);
			memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			shader->UnmapUniformBuffer(0);

			struct Light
			{
				glm::vec3 Direction;
				float Padding = 0.0f;
				glm::vec3 Radiance;
				float Multiplier;
			};

			struct UB
			{
				Light lights;
				glm::vec3 u_CameraPosition;
			};

			UB ub;
			ub.lights =
			{
				{ 0.5f, 0.5f, 0.5f },
				0.0f,
				{ 1.0f, 1.0f, 1.0f },
				1.0f
			};

			ub.lights.Direction = VulkanRendererHazelLegacy::GetLightDirectionTemp();
			ub.u_CameraPosition = cameraPosition;

			ubPtr = shader->MapUniformBuffer(1, 0);
			memcpy(ubPtr, &ub, sizeof(UB));
			shader->UnmapUniformBuffer(1, 0);
		}

		// Skybox
		s_Data.SkyboxMaterial->Set("u_Uniforms.TextureLod", s_Data.SceneData.SkyboxLod);
		s_Data.SkyboxMaterial->Set("u_Texture", s_Data.SceneData.SceneEnvironment.RadianceMap);
		RendererHazelLegacy::SubmitFullscreenQuad(Ref<RenderCommandBuffer>(), s_Data.SkyboxPipeline, Ref<UniformBufferSet>(), s_Data.SkyboxMaterial);

		// Render entities
		for (auto& dc : s_Data.DrawList)
		{
			RendererHazelLegacy::RenderMesh(s_Data.GeometryPipeline, dc.Mesh, dc.Transform);
		}

		for (auto& dc : s_Data.SelectedMeshDrawList)
		{
			RendererHazelLegacy::RenderMesh(s_Data.GeometryPipeline, dc.Mesh, dc.Transform);
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
			RendererHazelLegacy::RenderQuad(s_Data.GridPipeline, s_Data.GridMaterial, transform);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
			Renderer2D::BeginScene(viewProjection, glm::mat4(1.0f), true);
			for (auto& dc : s_Data.DrawList)
			{
				RendererHazelLegacy::DrawAABB(dc.Mesh, dc.Transform);
			}
			Renderer2D::EndScene();
		}

		RendererHazelLegacy::EndRenderPass(Ref<RenderCommandBuffer>());
	}

	void SceneRendererVulkan::CompositePass()
	{
		RendererHazelLegacy::BeginRenderPass(Ref<RenderCommandBuffer>(), s_Data.CompositePipeline->GetSpecification().RenderPass);

		float exposure = s_Data.SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples;

		s_Data.CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		// s_Data.CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		auto& framebuffer = s_Data.GeoPass->GetSpecification().TargetFramebuffer;
		auto vulkanFramebuffer = framebuffer.As<VulkanFramebufferHazelLegacy>();

		// s_Data.CompositeMaterial->Set("u_Texture", vulkanFramebuffer->GetVulkanDescriptorInfo()); // how it works?
		s_Data.CompositeMaterial->Set("u_Texture", vulkanFramebuffer->GetColorAttachmentRendererID());

		RendererHazelLegacy::SubmitFullscreenQuad(Ref<RenderCommandBuffer>(), s_Data.CompositePipeline, Ref<UniformBufferSet>(), s_Data.CompositeMaterial);
		RendererHazelLegacy::EndRenderPass(Ref<RenderCommandBuffer>());
	}

	void SceneRendererVulkan::BloomBlurPass()
	{
	}

	void SceneRendererVulkan::ShadowMapPass()
	{
	}

	void SceneRendererVulkan::FlushDrawList()
	{
		HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

		GeometryPass();
		CompositePass();
	}

	SceneRendererOptionsVulkan& SceneRendererVulkan::GetOptions()
	{
		return s_Data.Options;
	}

	void SceneRendererVulkan::SetLineWidth(float width)
	{
		m_LineWidth = width;

		//	TODO
		//	if (m_GeometryWireframePipeline)
		//	{
		//		m_GeometryWireframePipeline->GetSpecification().LineWidth = width;
		//	}
	}

}
