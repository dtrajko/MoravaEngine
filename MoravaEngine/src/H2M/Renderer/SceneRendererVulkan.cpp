#include "SceneRendererVulkan.h"

#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/RenderPass.h"
#include "H2M/Renderer/Renderer2D.h"

#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"


namespace H2M
{
	static std::vector<std::thread> s_ThreadPool;

	struct SceneRendererData
	{
		const HazelScene* ActiveScene = nullptr;
		struct SceneInfo
		{
			SceneRendererCameraVulkan SceneCamera;

			// Resources
			RefH2M<MaterialH2M> SkyboxMaterial;
			Environment SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironment SceneLightEnvironment;
			HazelDirLight ActiveLight;
			glm::vec3 LightDirectionTemp;
		} SceneData;

		RefH2M<Texture2D_H2M> BRDFLUT;
		RefH2M<HazelShader> CompositeShader;
		RefH2M<MaterialH2M> CompositeMaterial;
		RefH2M<MoravaShader> BloomBlurShader;
		RefH2M<MoravaShader> BloomBlendShader;

		RefH2M<RenderPass> GeoPass;
		RefH2M<RenderPass> CompositePass;
		RefH2M<RenderPass> BloomBlurPass[2];
		RefH2M<RenderPass> BloomBlendPass;

		RefH2M<Pipeline> GeometryPipeline;
		RefH2M<Pipeline> CompositePipeline;
		RefH2M<Pipeline> SkyboxPipeline;
		RefH2M<Pipeline> ShadowPassPipeline;
		RefH2M<MaterialH2M> SkyboxMaterial;

		struct DrawCommand
		{
			RefH2M<HazelMesh> Mesh;
			RefH2M<MaterialH2M> Material;
			glm::mat4 Transform;
		};

		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ColliderDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		// Grid
		RefH2M<Pipeline> GridPipeline;
		RefH2M<HazelShader> GridShader;
		RefH2M<MaterialH2M> GridMaterial;
		RefH2M<MaterialH2M> OutlineMaterial;
		RefH2M<MaterialH2M> OutlineAnimMaterial;

		SceneRendererOptionsVulkan Options;

		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		bool NeedsResize = false;

		VkDescriptorImageInfo ColorBufferInfo;
	};

	static SceneRendererData s_Data;

	SceneRendererVulkan::SceneRendererVulkan(RefH2M<HazelScene> scene, SceneRendererSpecificationVulkan specification)
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
		s_Data.BRDFLUT = Texture2D_H2M::Create("assets/textures/BRDF_LUT.tga");

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
			RefH2M<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);

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
			RefH2M<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);
			framebuffer->AddResizeCallback([](RefH2M<HazelFramebuffer> framebuffer)
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

	void SceneRendererVulkan::SetScene(RefH2M<HazelScene> scene)
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
		H2M_CORE_ASSERT(!s_Data.ActiveScene, "");

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

		RendererH2M::SetSceneEnvironment(RefH2M<SceneRendererH2M>(), &s_Data.SceneData.SceneEnvironment, RefH2M<HazelImage2D>(), RefH2M<HazelImage2D>());
	}

	void SceneRendererVulkan::EndScene()
	{
		H2M_CORE_ASSERT(s_Data.ActiveScene, "");

		s_Data.ActiveScene = nullptr;

		FlushDrawList();
#if MULTI_THREAD
		// RefH2M<SceneRendererVulkan> instance = this;
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

	void SceneRendererVulkan::SubmitMesh(MeshComponentH2M meshComponent, TransformComponent transformComponent)
	{
		SubmitMesh(meshComponent.Mesh, transformComponent.GetTransform(), RefH2M<MaterialH2M>());
	}

	void SceneRendererVulkan::SubmitSelectedMesh(MeshComponentH2M meshComponent, TransformComponent transformComponent)
	{
		SubmitSelectedMesh(meshComponent.Mesh, transformComponent.GetTransform());
	}

	void SceneRendererVulkan::SubmitMesh(RefH2M<HazelMesh> mesh, const glm::mat4& transform, RefH2M<MaterialH2M> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data.DrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRendererVulkan::SubmitSelectedMesh(RefH2M<HazelMesh> mesh, const glm::mat4& transform)
	{
		s_Data.SelectedMeshDrawList.push_back({ mesh, RefH2M<MaterialH2M>(), transform });
		// s_Data.ShadowPassDrawList.push_back({ mesh, RefH2M<MaterialH2M>, transform });
	}

	RefH2M<RenderPass> SceneRendererVulkan::GetFinalRenderPass()
	{
		return RefH2M<RenderPass>();
	}

	RefH2M<Texture2D_H2M> SceneRendererVulkan::GetFinalPassImage()
	{
		return RefH2M<Texture2D_H2M>();
	}

	void SceneRendererVulkan::GeometryPass()
	{
		RendererH2M::BeginRenderPass(RefH2M<RenderCommandBuffer>(), s_Data.GeoPass);

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

			ub.lights.Direction = VulkanRendererH2M::GetLightDirectionTemp();
			ub.u_CameraPosition = cameraPosition;

			ubPtr = shader->MapUniformBuffer(1, 0);
			memcpy(ubPtr, &ub, sizeof(UB));
			shader->UnmapUniformBuffer(1, 0);
		}

		// Skybox
		s_Data.SkyboxMaterial->Set("u_Uniforms.TextureLod", s_Data.SceneData.SkyboxLod);
		s_Data.SkyboxMaterial->Set("u_Texture", s_Data.SceneData.SceneEnvironment.RadianceMap);
		RendererH2M::SubmitFullscreenQuad(RefH2M<RenderCommandBuffer>(), s_Data.SkyboxPipeline, RefH2M<UniformBufferSet>(), s_Data.SkyboxMaterial);

		// Render entities
		for (auto& dc : s_Data.DrawList)
		{
			RendererH2M::RenderMesh(s_Data.GeometryPipeline, dc.Mesh, dc.Transform);
		}

		for (auto& dc : s_Data.SelectedMeshDrawList)
		{
			RendererH2M::RenderMesh(s_Data.GeometryPipeline, dc.Mesh, dc.Transform);
		}

		// Grid
		if (GetOptions().ShowGrid)
		{
			const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f));
			RendererH2M::RenderQuad(s_Data.GridPipeline, s_Data.GridMaterial, transform);
		}

		if (GetOptions().ShowBoundingBoxes)
		{
			Renderer2D::BeginScene(viewProjection, glm::mat4(1.0f), true);
			for (auto& dc : s_Data.DrawList)
			{
				RendererH2M::DrawAABB(dc.Mesh, dc.Transform);
			}
			Renderer2D::EndScene();
		}

		RendererH2M::EndRenderPass(RefH2M<RenderCommandBuffer>());
	}

	void SceneRendererVulkan::CompositePass()
	{
		RendererH2M::BeginRenderPass(RefH2M<RenderCommandBuffer>(), s_Data.CompositePipeline->GetSpecification().RenderPass);

		float exposure = s_Data.SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples;

		s_Data.CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		// s_Data.CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		auto& framebuffer = s_Data.GeoPass->GetSpecification().TargetFramebuffer;
		auto vulkanFramebuffer = framebuffer.As<VulkanFramebufferH2M>();

		// s_Data.CompositeMaterial->Set("u_Texture", vulkanFramebuffer->GetVulkanDescriptorInfo()); // how it works?
		s_Data.CompositeMaterial->Set("u_Texture", vulkanFramebuffer->GetColorAttachmentRendererID());

		RendererH2M::SubmitFullscreenQuad(RefH2M<RenderCommandBuffer>(), s_Data.CompositePipeline, RefH2M<UniformBufferSet>(), s_Data.CompositeMaterial);
		RendererH2M::EndRenderPass(RefH2M<RenderCommandBuffer>());
	}

	void SceneRendererVulkan::BloomBlurPass()
	{
	}

	void SceneRendererVulkan::ShadowMapPass()
	{
	}

	void SceneRendererVulkan::FlushDrawList()
	{
		H2M_CORE_ASSERT(!s_Data.ActiveScene, "");

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
