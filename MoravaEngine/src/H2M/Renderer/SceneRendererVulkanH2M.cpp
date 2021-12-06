/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "SceneRendererVulkanH2M.h"

#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"


namespace H2M {

	struct SceneRendererData
	{
		const SceneH2M* ActiveScene = nullptr;
		struct SceneInfo
		{
			SceneRendererCameraH2M SceneCamera;

			// Resources
			RefH2M<MaterialH2M> SkyboxMaterial;
			EnvironmentH2M SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
			LightEnvironmentH2M SceneLightEnvironment;
			LightH2M ActiveLight;
			glm::vec3 LightDirectionTemp;
		} SceneData;

		RefH2M<Texture2D_H2M> BRDFLUT;
		RefH2M<ShaderH2M> CompositeShader;
		RefH2M<MaterialH2M> CompositeMaterial;
		RefH2M<MoravaShader> BloomBlurShader;
		RefH2M<MoravaShader> BloomBlendShader;

		RefH2M<RenderPassH2M> GeoPass;
		RefH2M<RenderPassH2M> CompositePass;
		RefH2M<RenderPassH2M> BloomBlurPass[2];
		RefH2M<RenderPassH2M> BloomBlendPass;

		RefH2M<PipelineH2M> GeometryPipeline;
		RefH2M<PipelineH2M> CompositePipeline;
		RefH2M<PipelineH2M> SkyboxPipeline;
		RefH2M<PipelineH2M> ShadowPassPipeline;
		RefH2M<MaterialH2M> SkyboxMaterial;

		struct DrawCommand
		{
			RefH2M<MeshH2M> Mesh;
			RefH2M<MaterialH2M> Material;
			glm::mat4 Transform;
		};

		std::vector<DrawCommand> DrawList;
		std::vector<DrawCommand> SelectedMeshDrawList;
		std::vector<DrawCommand> ColliderDrawList;
		std::vector<DrawCommand> ShadowPassDrawList;

		// Grid
		RefH2M<PipelineH2M> GridPipeline;
		RefH2M<ShaderH2M> GridShader;
		RefH2M<MaterialH2M> GridMaterial;
		RefH2M<MaterialH2M> OutlineMaterial;
		RefH2M<MaterialH2M> OutlineAnimMaterial;

		SceneRendererOptionsH2M Options;

		uint32_t ViewportWidth = 0;
		uint32_t ViewportHeight = 0;
		bool NeedsResize = false;

		VkDescriptorImageInfo ColorBufferInfo;
	};

	static SceneRendererData s_Data;

	void SceneRendererVulkanH2M::Init()
	{
		FramebufferSpecificationH2M geoFramebufferSpec = {};
		geoFramebufferSpec.Width = 1280;
		geoFramebufferSpec.Height = 720;
		geoFramebufferSpec.Format = FramebufferFormatH2M::RGBA16F;
		geoFramebufferSpec.Samples = 8;
		geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecificationH2M geoRenderPassSpec = {};
		geoRenderPassSpec.TargetFramebuffer = FramebufferH2M::Create(geoFramebufferSpec);
		/****
		s_Data.GeoPass = RenderPass::Create(geoRenderPassSpec);

		FramebufferSpecificationH2M compFramebufferSpec = {};
		compFramebufferSpec.Width = 1280;
		compFramebufferSpec.Height = 720;
		compFramebufferSpec.Format = FramebufferFormat::RGBA8;
		compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

		RenderPassSpecification compRenderPassSpec;
		compRenderPassSpec.TargetFramebuffer = HazelFramebuffer::Create(compFramebufferSpec);
		s_Data.CompositePass = RenderPass::Create(compRenderPassSpec);

		s_Data.CompositeShader = RendererH2M::GetShaderLibrary()->Get("SceneComposite");
		s_Data.CompositeMaterial = HazelMaterial::Create(s_Data.CompositeShader, "CompositeMaterial");
		s_Data.BRDFLUT = HazelTexture2D::Create("assets/textures/BRDF_LUT.tga");

		// Grid pipeline
		{
			s_Data.GridShader = RendererH2M::GetShaderLibrary()->Get("Grid");
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
		auto outlineShader = RendererH2M::GetShaderLibrary()->Get("Outline");
		s_Data.OutlineMaterial = HazelMaterial::Create(outlineShader);
		s_Data.OutlineMaterial->SetFlag(HazelMaterialFlag::DepthTest, false);

		// Skybox pipeline
		{
			auto skyboxShader = RendererH2M::GetShaderLibrary()->Get("Skybox");

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
			FramebufferSpecificationH2M spec = {};;
			Ref<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);

			PipelineSpecification pipelineSpecification = {};
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = RendererH2M::GetShaderLibrary()->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec = {};
			renderPassSpec.TargetFramebuffer = framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "PBR-Static";
			// s_Data.GeometryPipeline = Pipeline::Create(pipelineSpecification); // fragment shader writes to output location 1 with no matching attachment
		}

		// Composite pipeline
		{
			FramebufferSpecificationH2M spec = {};;
			Ref<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);
			framebuffer->AddResizeCallback([](Ref<HazelFramebuffer> framebuffer)
			{
				// RendererH2M::Submit([framebuffer]() mutable {});
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
			pipelineSpecification.Shader = RendererH2M::GetShaderLibrary()->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			// s_Data.CompositePipeline = Pipeline::Create(pipelineSpecification); // fragment shader writes to output location 1 with no matching attachment
		}
		****/
	}

	void SceneRendererVulkanH2M::SetViewportSize(uint32_t width, uint32_t height)
	{
		s_Data.ViewportWidth = width;
		s_Data.ViewportHeight = height;
		s_Data.NeedsResize = true;
	}

	void SceneRendererVulkanH2M::BeginScene(const SceneH2M* scene, const SceneRendererCameraH2M& camera)
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

		RendererH2M::SetSceneEnvironment(&s_Data.SceneData.SceneEnvironment, RefH2M<Image2D_H2M>());
	}

	void SceneRendererVulkanH2M::EndScene()
	{
		H2M_CORE_ASSERT(s_Data.ActiveScene, "");

		s_Data.ActiveScene = nullptr;

		FlushDrawList();
	}

	void SceneRendererVulkanH2M::SubmitMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent)
	{
		SubmitMesh(meshComponent.Mesh, transformComponent.GetTransform(), RefH2M<MaterialH2M>());
	}

	void SceneRendererVulkanH2M::SubmitSelectedMesh(MeshComponentH2M meshComponent, TransformComponentH2M transformComponent)
	{
		SubmitSelectedMesh(meshComponent.Mesh, transformComponent.GetTransform());
	}

	void SceneRendererVulkanH2M::SubmitMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform, RefH2M<MaterialH2M> overrideMaterial)
	{
		// TODO: Culling, sorting, etc.
		s_Data.DrawList.push_back({ mesh, overrideMaterial, transform });
	}

	void SceneRendererVulkanH2M::SubmitSelectedMesh(RefH2M<MeshH2M> mesh, const glm::mat4& transform)
	{
		s_Data.SelectedMeshDrawList.push_back({ mesh, RefH2M<MaterialH2M>(), transform });
		// s_Data.ShadowPassDrawList.push_back({ mesh, Ref<HazelMaterial>, transform });
	}

	void SceneRendererVulkanH2M::GeometryPass()
	{
		RendererH2M::BeginRenderPass(s_Data.GeoPass);

		auto viewProjection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.ViewMatrix;
		glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.ViewMatrix)[3];
		// glm::vec3 cameraPosition = s_Data.SceneData.SceneCamera.Camera.GetPosition();

		// RendererH2M::Submit([viewProjection, cameraPosition]() {});
		{
			auto inverseVP = glm::inverse(viewProjection);
			auto shader = s_Data.GridMaterial->GetShader().As<VulkanShaderH2M>();
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

			shader = s_Data.SkyboxMaterial->GetShader().As<VulkanShaderH2M>();
			ubPtr = shader->MapUniformBuffer(0);
			memcpy(ubPtr, &viewProj, sizeof(ViewProj));
			shader->UnmapUniformBuffer(0);

			shader = s_Data.GeometryPipeline->GetSpecification().Shader.As<VulkanShaderH2M>();
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
		RendererH2M::SubmitFullscreenQuad(s_Data.SkyboxPipeline, s_Data.SkyboxMaterial);

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
			Renderer2D_H2M::BeginScene(viewProjection, true);
			for (auto& dc : s_Data.DrawList)
			{
				RendererH2M::DrawAABB(dc.Mesh, dc.Transform);
			}
			Renderer2D_H2M::EndScene();
		}

		RendererH2M::EndRenderPass();
	}

	void SceneRendererVulkanH2M::CompositePass()
	{
		RendererH2M::BeginRenderPass(s_Data.CompositePipeline->GetSpecification().RenderPass);

		float exposure = s_Data.SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples;

		s_Data.CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		// s_Data.CompositeMaterial->Set("u_Uniforms.TextureSamples", textureSamples);

		auto& framebuffer = s_Data.GeoPass->GetSpecification().TargetFramebuffer;
		auto vulkanFramebuffer = framebuffer.As<VulkanFramebufferH2M>();

		// s_Data.CompositeMaterial->Set("u_Texture", vulkanFramebuffer->GetVulkanDescriptorInfo()); // how it works?
		s_Data.CompositeMaterial->Set("u_Texture", vulkanFramebuffer->GetColorAttachmentRendererID());

		RendererH2M::SubmitFullscreenQuad(s_Data.CompositePipeline, s_Data.CompositeMaterial);
		RendererH2M::EndRenderPass();
	}

	void SceneRendererVulkanH2M::FlushDrawList()
	{
		H2M_CORE_ASSERT(!s_Data.ActiveScene, "");

		GeometryPass();
		CompositePass();
	}

	SceneRendererOptionsH2M& SceneRendererVulkanH2M::GetOptions()
	{
		return s_Data.Options;
	}

}
