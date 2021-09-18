#include "VulkanRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
#include "Hazel/Platform/Vulkan/VulkanComputePipeline.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanMaterial.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTestLayer.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "Platform/Vulkan/VulkanSkyboxCube.h"

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan_with_textures.h"

#include "ImGuizmo.h"


namespace Hazel {

	namespace Utils
	{
		static const char* VulkanVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
			case 0x10DE: return "NVIDIA";
			case 0x1002: return "AMD";
			case 0x8086: return "INTEL";
			case 0x13B5: return "ARM";
			}
			return "Unknown";
		}
	}

	bool VulkanRenderer::s_MipMapsEnabled = true;
	bool VulkanRenderer::s_ViewportFBNeedsResize = false;

	static VkCommandBuffer s_ImGuiCommandBuffer;
	static VkCommandBuffer s_CompositeCommandBuffer;

	static Ref<HazelFramebuffer> s_Framebuffer;
	static Ref<HazelFramebuffer> s_CompositeFramebuffer;
	static Ref<Pipeline> s_MeshPipeline;
	static Ref<Pipeline> s_CompositePipeline;
	static Ref<VertexBuffer> s_QuadVertexBuffer;
	static Ref<IndexBuffer> s_QuadIndexBuffer;
	static VulkanShader::ShaderMaterialDescriptorSet s_QuadDescriptorSet;
	static ImTextureID s_TextureID;
	static uint32_t s_ViewportWidth = 1280;
	static uint32_t s_ViewportHeight = 720;

	static std::vector<Ref<HazelMesh>> s_Meshes;

	static Submesh* s_SelectedSubmesh;
	static glm::mat4* s_Transform_ImGuizmo = nullptr;

	struct VulkanRendererData
	{
		VkCommandBuffer ActiveCommandBuffer = nullptr;
		Ref<HazelTexture2D> BRDFLut;
		VulkanShader::ShaderMaterialDescriptorSet RendererDescriptorSetFeb2021;
		// std::unordered_map<SceneRenderer*, std::vector<VulkanShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;

		float Exposure = 0.8f;

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;
			Environment SceneEnvironment;
			float SkyboxLod;
			glm::vec3 LightDirectionTemp;
		} SceneData;

		std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> EnvironmentMap;

		/**** BEGIN dtrajko Keep smart references alive ****/
		Ref<HazelTextureCube> envUnfiltered;
		Ref<HazelTexture2D> envEquirect;
		Ref<HazelTextureCube> envFiltered;
		Ref<HazelTextureCube> irradianceMap;
		/**** END dtrajko Keep smart references alive ****/

		RendererCapabilities RenderCaps;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VulkanShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

		VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> DescriptorPools;
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		// std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		// std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

		// Default samplers
		VkSampler SamplerClamp = nullptr;

		int32_t SelectedDrawCall = -1;
		int32_t DrawCallCount = 0;

		Ref<HazelShaderLibrary> m_ShaderLibrary;

		// dtrajko vulkan skybox
		Ref<VulkanSkyboxCube> VulkanSkyboxCube;
		Ref<Pipeline> SkyboxPipeline;
		Ref<HazelShader> SkyboxShader;
	};

	static VulkanRendererData s_Data;

	void VulkanRenderer::SubmitMesh(const Ref<HazelMesh>& mesh)
	{
		// Temporary code - populate selected submesh
		std::vector<Submesh> submeshes = mesh->GetSubmeshes();
		s_SelectedSubmesh = &submeshes.at(0);

		s_Meshes.push_back(mesh);
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		// HazelRenderer::Submit([=]() {});
		{
			auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>();

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = *s_QuadDescriptorSet.DescriptorSets.data();
			writeDescriptorSet.descriptorCount = (uint32_t)s_QuadDescriptorSet.DescriptorSets.size();
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
			writeDescriptorSet.dstBinding = 0;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	void VulkanRenderer::Init()
	{
		// HazelRenderer::Submit([=]() {});
		{
			s_ImGuiCommandBuffer = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
			s_CompositeCommandBuffer = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
		}

		s_Data = VulkanRendererData{};
		auto& caps = s_Data.RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);

		Utils::DumpGPUInfo();

		// TODO: Create descriptor pools

		/**** BEGIN code from HazelRenderer::Init() ****/

		s_Data.m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		// s_Data.m_ShaderLibrary->Load("assets/shaders/Grid.glsl");
		// s_Data.m_ShaderLibrary->Load("assets/shaders/SceneComposite.glsl");
		// s_Data.m_ShaderLibrary->Load("assets/shaders/HazelSimple.glsl");
		// s_Data.m_ShaderLibrary->Load("assets/shaders/Outline.glsl");
		s_Data.m_ShaderLibrary->Load("assets/shaders/Skybox.glsl");
		s_Data.m_ShaderLibrary->Load("assets/shaders/HazelPBR_Static.glsl");
		s_Data.m_ShaderLibrary->Load("assets/shaders/Texture.glsl");
		s_Data.m_ShaderLibrary->Load("assets/shaders/SceneComposite.glsl");

		SceneRenderer::Init();

		// Renderer2D::Init();

		/**** END code from HazelRenderer::Init() ****/

		{
			HazelFramebufferSpecification spec;
			spec.DebugName = "Viewport";
			spec.Width = s_ViewportWidth;
			spec.Height = s_ViewportHeight;
			s_Framebuffer = HazelFramebuffer::Create(spec);
			s_Framebuffer->AddResizeCallback([](Ref<HazelFramebuffer> framebuffer)
			{
				// HazelRenderer::Submit([framebuffer]() mutable {});
				{
					auto vulkanFB = framebuffer.As<VulkanFramebuffer>();
					const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
					Log::GetLogger()->warn("Resizing framebuffer; image layout is {0}", imageInfo.imageLayout);
					// s_TextureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
					s_TextureID = ImGui_ImplVulkan_UpdateTextureInfo((VkDescriptorSet)s_TextureID, imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
				}
			});

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = s_Data.m_ShaderLibrary->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = s_Framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);

			s_MeshPipeline = Pipeline::Create(pipelineSpecification);
		}

		{
			HazelFramebufferSpecification spec;
			spec.DebugName = "CompositeFramebuffer";
			spec.SwapChainTarget = true;
			spec.Width = s_ViewportWidth;
			spec.Height = s_ViewportHeight;
			s_CompositeFramebuffer = HazelFramebuffer::Create(spec);
			s_CompositeFramebuffer->AddResizeCallback([](Ref<HazelFramebuffer> framebuffer)
			{
				// HazelRenderer::Submit([framebuffer]() mutable {});
				{
					auto vulkanFB = framebuffer.As<VulkanFramebuffer>();
					const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
					HZ_CORE_WARN("Resizing framebuffer; image layout is {0}", imageInfo.imageLayout);
					s_TextureID = ImGui_ImplVulkan_UpdateTextureInfo((VkDescriptorSet)s_TextureID, imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
				}
			});

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = s_Data.m_ShaderLibrary->Get("SceneComposite");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = s_CompositeFramebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
			pipelineSpecification.DebugName = "SceneComposite";
			s_CompositePipeline = Pipeline::Create(pipelineSpecification);
		}

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

		s_QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
		s_QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		// HazelRenderer::Submit([=]() {});
		{
			auto shader = s_CompositePipeline->GetSpecification().Shader.As<VulkanShader>();
			auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>();
			s_QuadDescriptorSet = shader->CreateDescriptorSets();

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = *s_QuadDescriptorSet.DescriptorSets.data();
			writeDescriptorSet.descriptorCount = (uint32_t)s_QuadDescriptorSet.DescriptorSets.size();
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
			writeDescriptorSet.dstBinding = 0;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);

			auto vulkanFB = s_Framebuffer.As<VulkanFramebuffer>();
			const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
			s_TextureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
		}

		// s_Data.EnvironmentMap = CreateEnvironmentMap("Textures/HDR/pink_sunrise_4k.hdr");
		// s_Data.EnvironmentMap = CreateEnvironmentMap("Textures/HDR/umhlanga_sunrise_4k.hdr");
		// s_Data.EnvironmentMap = CreateEnvironmentMap("Textures/HDR/venice_dawn_1_4k.hdr");
		s_Data.EnvironmentMap = CreateEnvironmentMap("Textures/HDR/newport_loft.hdr");

		s_Data.BRDFLut = HazelTexture2D::Create("assets/textures/BRDF_LUT.tga");

		// HazelRenderer::Submit([environment]() mutable {});
		{
			auto shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");
			Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();
			s_Data.RendererDescriptorSetFeb2021 = pbrShader->CreateDescriptorSets(1);
		}

		SetSceneEnvironmentStatic(Ref<Environment>::Create(s_Data.EnvironmentMap.first, s_Data.EnvironmentMap.second), Ref<HazelImage2D>());

		/*** BEGIN Setup the Skybox ****/
		s_Data.VulkanSkyboxCube = Hazel::Ref<VulkanSkyboxCube>::Create();

		PipelineSpecification skyboxPipelineSpecification;
		skyboxPipelineSpecification.DebugName = "Skybox Pipeline Specification";
		skyboxPipelineSpecification.Layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		};
		s_Data.SkyboxShader = s_Data.m_ShaderLibrary->Get("Skybox");
		skyboxPipelineSpecification.Shader = s_Data.SkyboxShader;

		RenderPassSpecification renderPassSpecification;
		renderPassSpecification.DebugName = "Skybox RenderPass Specification";
		renderPassSpecification.TargetFramebuffer = s_Framebuffer;
		skyboxPipelineSpecification.RenderPass = RenderPass::Create(renderPassSpecification);

		s_Data.SkyboxPipeline = Pipeline::Create(skyboxPipelineSpecification);
		/*** END Setup the Skybox ****/

		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

		s_Data.SceneData.SkyboxLod = 0.0f;
		s_Data.SceneData.LightDirectionTemp = { 0.5f, 0.5f, 0.5f };

		OnResize(s_ViewportWidth, s_ViewportHeight);
	}

	void VulkanRenderer::Shutdown()
	{
		VulkanShader::ClearUniformBuffers();
		// delete s_Data;
	}

	void VulkanRenderer::RenderMeshVulkan(Ref<HazelMesh> mesh, VkCommandBuffer commandBuffer)
	{
		/**** BEGIN keep smart references alive ****/
		Ref<HazelTextureCube> envUnfiltered = s_Data.envUnfiltered;
		Ref<HazelTextureCube> envFiltered = s_Data.envFiltered;
		Ref<HazelTextureCube> irradianceMap = s_Data.irradianceMap;
		std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> environmentMap = s_Data.EnvironmentMap;
		Ref<HazelTexture2D> BRDFLut = s_Data.BRDFLut;
		Ref<HazelTexture2D> envEquirect = s_Data.envEquirect;

		// auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// auto shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");
		// Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();

		// std::array<VkWriteDescriptorSet, 1> writeDescriptors;

		// writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_AlbedoTexture", 0);
		// writeDescriptors[0].dstSet = pbrShader->CreateDescriptorSets(0).DescriptorSet;
		// writeDescriptors[0].pBufferInfo = &pbrShader->GetUniformBuffer(2, 0).Descriptor;

		// vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		/**** END keep smart references alive ****/

		/**** BEGIN Non-composite ****
		Ref<VulkanPipeline> vulkanPipeline = mesh->GetPipeline().As<VulkanPipeline>();
		/**** END Non-composite ****/
		/**** BEGIN Composite ****/
		Ref<VulkanPipeline> vulkanPipeline = s_MeshPipeline.As<VulkanPipeline>();
		/**** END Composite ****/

		VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

		auto vulkanMeshVB = mesh->GetVertexBuffer().As<VulkanVertexBuffer>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = Ref<VulkanIndexBuffer>(mesh->GetIndexBuffer());
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		auto& submeshes = mesh->GetSubmeshes();
		for (Submesh& submesh : submeshes)
		{
			auto& material = mesh->GetMaterials()[submesh.MaterialIndex];
			Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			std::vector<VkDescriptorSet> descriptorSet = mesh->GetDescriptorSet(submesh.MaterialIndex).DescriptorSet.DescriptorSets;
			VulkanShader::ShaderMaterialDescriptorSet rendererDescriptorSet = s_Data.RendererDescriptorSetFeb2021;

			std::array<VkDescriptorSet, 2> descriptorSets = {
				*descriptorSet.data(),
				*rendererDescriptorSet.DescriptorSets.data(),
			};

			// VkDescriptorSet* descriptorSet = (VkDescriptorSet*)mesh->GetDescriptorSet();
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

			// Push Constants
			// glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			// vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submesh.Transform);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
		}
	}

	void VulkanRenderer::RenderSkybox(VkCommandBuffer commandBuffer)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		Ref<VulkanPipeline> vulkanSkyboxPipeline = s_Data.SkyboxPipeline.As<VulkanPipeline>();

		VkPipelineLayout skyboxPipelineLayout = vulkanSkyboxPipeline->GetVulkanPipelineLayout();

		Ref<VulkanShader> vulkanSkyboxShader = s_Data.SkyboxShader.As<VulkanShader>();

		void* ubPtr = vulkanSkyboxShader->MapUniformBuffer(0, 0);
		struct SkyboxUniformCamera
		{
			glm::mat4 ViewProjectionMatrix;  // u_ViewProjectionMatrix
			glm::mat4 InverseViewProjection; // u_InverseViewProjection
		} skyboxUniformCamera;
		
		skyboxUniformCamera.ViewProjectionMatrix = s_Data.SceneData.SceneCamera.Camera.GetViewProjection();
		skyboxUniformCamera.InverseViewProjection = glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewProjection());
		memcpy(ubPtr, &skyboxUniformCamera, sizeof(SkyboxUniformCamera));
		vulkanSkyboxShader->UnmapUniformBuffer(0, 0);

		std::array<VkWriteDescriptorSet, 2> writeDescriptors;
		VulkanShader::ShaderMaterialDescriptorSet descriptorSet = vulkanSkyboxShader->CreateDescriptorSets();

		writeDescriptors[0] = *vulkanSkyboxShader->GetDescriptorSet("Camera");
		writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
		writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[0].pBufferInfo = &vulkanSkyboxShader->GetUniformBuffer(0, 0).Descriptor;

		// Ref<VulkanTextureCube> envUnfilteredCubemap = s_Data.envUnfiltered.As<VulkanTextureCube>();
		Ref<VulkanTextureCube> envFilteredCubemap = s_Data.envFiltered.As<VulkanTextureCube>();
		writeDescriptors[1] = *vulkanSkyboxShader->GetDescriptorSet("u_Texture");
		writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
		writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
		writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

		vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

		Ref<VulkanVertexBuffer> vulkanSkyboxCubeVB = s_Data.VulkanSkyboxCube->m_VertexBuffer.As<VulkanVertexBuffer>();
		VkBuffer skyboxCubeVertexVkBuffer = vulkanSkyboxCubeVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &skyboxCubeVertexVkBuffer, offsets);

		Ref<VulkanIndexBuffer> vulkanSkyboxCubeIB = s_Data.VulkanSkyboxCube->m_IndexBuffer.As<VulkanIndexBuffer>();
		VkBuffer skyboxCubeIndexVkBuffer = vulkanSkyboxCubeIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, skyboxCubeIndexVkBuffer, 0, VK_INDEX_TYPE_UINT32);

		VkPipeline skyboxPipeline = vulkanSkyboxPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, (uint32_t)descriptorSet.DescriptorSets.size(), descriptorSet.DescriptorSets.data(), 0, nullptr);

		// push constants
		vkCmdPushConstants(commandBuffer, skyboxPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &s_Data.SceneData.SkyboxLod);

		vkCmdDrawIndexed(commandBuffer, s_Data.VulkanSkyboxCube->m_IndexCount, 1, 0, 0, 0);
	}

	// TODO: virtual or static?
	void VulkanRenderer::BeginFrame()
	{
		Log::GetLogger()->error("The virtual method BeginFrame currently not in use. Use BeginFrameStatic instead!");
	}

	// TODO: virtual or static?
	void VulkanRenderer::BeginFrameStatic()
	{
		// HazelRenderer::Submit([]() {});
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
			s_Data.ActiveCommandBuffer = drawCommandBuffer;
			HZ_CORE_ASSERT(s_Data.ActiveCommandBuffer);
			// VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo)); // commandBuffer must not be in the recording or pending state
		}
	}

	void VulkanRenderer::EndFrame()
	{
		Log::GetLogger()->error("The virtual method EndFrame currently not in use. Use EndFrameStatic instead!");
	}

	void VulkanRenderer::EndFrameStatic()
	{
		// HazelRenderer::Submit([]() {});
		{
			// VK_CHECK_RESULT(vkEndCommandBuffer(s_Data.ActiveCommandBuffer));
			s_Data.ActiveCommandBuffer = nullptr;
		}
	}

	// TODO: virtual or static?
	void VulkanRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass)
	{
		Log::GetLogger()->error("The virtual method BeginRenderPass currently not in use. Use BeginRenderPassStatic instead!");
	}

	// TODO: virtual or static?
	void VulkanRenderer::BeginRenderPassStatic(const Ref<RenderPass>& renderPass)
	{
		// HazelRenderer::Submit([renderPass]() {});
		{
			// HZ_CORE_ASSERT(s_Data.ActiveCommandBuffer);

			auto fb = renderPass->GetSpecification().TargetFramebuffer;
			Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
			const auto& fbSpec = framebuffer->GetSpecification();

			uint32_t width = framebuffer->GetWidth();
			uint32_t height = framebuffer->GetHeight();

			BeginFrameStatic();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;

			// TODO: Does out framebuffer has a depth attachment?
			VkClearValue clearValues[2];
			clearValues[0].color = { { fbSpec.ClearColor.r, fbSpec.ClearColor.g, fbSpec.ClearColor.b, fbSpec.ClearColor.a } };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassBeginInfo.clearValueCount = 2; // Color + depth
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

			// vkCmdBeginRenderPass(s_Data.ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE); // You must call vkBeginCommandBuffer() before this call to vkCmdBeginRenderPass()

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			// vkCmdSetViewport(s_Data.ActiveCommandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			// vkCmdSetScissor(s_Data.ActiveCommandBuffer, 0, 1, &scissor);

			EndFrameStatic();
		}
	}

	// TODO: virtual or static?
	void VulkanRenderer::EndRenderPass()
	{
		Log::GetLogger()->error("The virtual method EndRenderPass currently not in use. Use EndRenderPassStatic instead!");
	}

	// TODO: virtual or static?
	void VulkanRenderer::EndRenderPassStatic()
	{
		// HazelRenderer::Submit([]() {});
		{
			// vkCmdEndRenderPass(s_Data.ActiveCommandBuffer);
			s_Data.ActiveCommandBuffer = nullptr;
		}
	}

	void VulkanRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
		Log::GetLogger()->error("The virtual method SetSceneEnvironment currently not in use. Use SetSceneEnvironmentStatic instead!");
	}

	void VulkanRenderer::SetSceneEnvironmentStatic(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
		// HazelRenderer::Submit([environment]() mutable {});
		{
			auto shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");
			Ref<VulkanShader> pbrShader = shader.As<VulkanShader>();

			std::array<VkWriteDescriptorSet, 3> writeDescriptors;

			Ref<VulkanTextureCube> radianceMap = environment->RadianceMap.As<VulkanTextureCube>();
			Ref<VulkanTextureCube> irradianceMap = environment->IrradianceMap.As<VulkanTextureCube>();

			writeDescriptors[0] = *pbrShader->GetDescriptorSet("u_EnvRadianceTex", 1);
			writeDescriptors[0].dstSet = *s_Data.RendererDescriptorSetFeb2021.DescriptorSets.data();
			writeDescriptors[0].descriptorCount = (uint32_t)s_Data.RendererDescriptorSetFeb2021.DescriptorSets.size();
			auto& radianceMapImageInfo = radianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[0].pImageInfo = &radianceMapImageInfo;

			writeDescriptors[1] = *pbrShader->GetDescriptorSet("u_EnvIrradianceTex", 1);
			writeDescriptors[1].dstSet = *s_Data.RendererDescriptorSetFeb2021.DescriptorSets.data();
			writeDescriptors[1].descriptorCount = (uint32_t)s_Data.RendererDescriptorSetFeb2021.DescriptorSets.size();
			auto& irradianceMapImageInfo = irradianceMap->GetVulkanDescriptorInfo();
			writeDescriptors[1].pImageInfo = &irradianceMapImageInfo;

			writeDescriptors[2] = *pbrShader->GetDescriptorSet("u_BRDFLUTTexture", 1);
			writeDescriptors[2].dstSet = *s_Data.RendererDescriptorSetFeb2021.DescriptorSets.data();
			writeDescriptors[2].descriptorCount = (uint32_t)s_Data.RendererDescriptorSetFeb2021.DescriptorSets.size();
			auto& brdfLutImageInfo = s_Data.BRDFLut.As<VulkanTexture2D>()->GetVulkanDescriptorInfo();
			writeDescriptors[2].pImageInfo = &brdfLutImageInfo;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
		}
	}

	void VulkanRenderer::GeometryPass()
	{
		// HazelRenderer::Submit([=]() {});
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));

			Ref<VulkanFramebuffer> framebuffer = s_Framebuffer.As<VulkanFramebuffer>();

			uint32_t width = framebuffer->GetWidth();
			uint32_t height = framebuffer->GetHeight();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;

			VkClearValue clearValues[2];
			clearValues[0].color = { {0.1f, 0.1f,0.1f, 1.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassBeginInfo.clearValueCount = 2; // Color + depth
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

			vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(drawCommandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(drawCommandBuffer, 0, 1, &scissor);

			VulkanRenderer::RenderSkybox(drawCommandBuffer); // in progress

			for (auto& mesh : s_Meshes)
			{
				RenderMeshVulkan(mesh, drawCommandBuffer);
			}

			s_Meshes.clear();

			vkCmdEndRenderPass(drawCommandBuffer);
		}
	}

	void VulkanRenderer::CompositePass()
	{
		// HazelRenderer::Submit([=]() {});
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();
			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			Ref<VulkanFramebuffer> framebuffer = s_CompositeFramebuffer.As<VulkanFramebuffer>();

			// uint32_t width = framebuffer->GetWidth();   // framebuffer resize still not enabled
			// uint32_t height = framebuffer->GetHeight(); // framebuffer resize still not enabled

			uint32_t width = swapChain.GetWidth();
			uint32_t height = swapChain.GetHeight();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			// renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;

			VkClearValue clearValues[2];
			clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassBeginInfo.clearValueCount = 2; // Color + depth
			renderPassBeginInfo.pClearValues = clearValues;
			// renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
			// vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkCommandBuffer commandBuffer = s_CompositeCommandBuffer;

			VkCommandBufferInheritanceInfo inheritanceInfo = {};
			inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.renderPass = swapChain.GetRenderPass();
			inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			std::vector<VkCommandBuffer> commandBuffers;

			// VkCommandBufferBeginInfo cmdBufInfo = {};
			// cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
			cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

			VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = (float)height;
			viewport.height = -(float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// Copy 3D scene here!
			Ref<VulkanPipeline> vulkanPipeline = s_CompositePipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				
			/**** BEGIN CompositeRenderPass(inheritanceInfo) ****/

			auto vulkanMeshVB = s_QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			float exposure = s_Data.Exposure;
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &exposure);

			// Bind descriptor sets describing shader binding points
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)s_QuadDescriptorSet.DescriptorSets.size(), s_QuadDescriptorSet.DescriptorSets.data(), 0, nullptr);

			vkCmdDrawIndexed(commandBuffer, s_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);

			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

			/**** END CompositeRenderPass(inheritanceInfo) ****/

			commandBuffers.push_back(s_CompositeCommandBuffer);

			OnImGuiRender(inheritanceInfo, commandBuffers);

			vkCmdExecuteCommands(drawCommandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

			vkCmdEndRenderPass(drawCommandBuffer);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
		}
	}

	void VulkanRenderer::OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers)
	{
		Ref<VulkanContext> context = VulkanContext::Get();
		VulkanSwapChain& swapChain = context->GetSwapChain();

		uint32_t width = swapChain.GetWidth();
		uint32_t height = swapChain.GetHeight();

		// ImGui Pass
		{
			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
			cmdBufInfo.pInheritanceInfo = &inheritanceInfo;

			VK_CHECK_RESULT(vkBeginCommandBuffer(s_ImGuiCommandBuffer, &cmdBufInfo));

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = (float)height;
			viewport.height = -(float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(s_ImGuiCommandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(s_ImGuiCommandBuffer, 0, 1, &scissor);

			// ImGui Dockspace
			bool open = true;
			bool* p_open = &open;

			static bool opt_fullscreen_persistant = true;
			bool opt_fullscreen = opt_fullscreen_persistant;
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
			// because it would be confusing to have two docking targets within each others.
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			if (opt_fullscreen)
			{
				ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->GetWorkPos());
				ImGui::SetNextWindowSize(viewport->GetWorkSize());
				ImGui::SetNextWindowViewport(viewport->ID);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
				window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
				window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
			}

			// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
			// and handle the pass-thru hole, so we ask Begin() to not render a background.
			if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				window_flags |= ImGuiWindowFlags_NoBackground;

			// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
			// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
			// all active windows docked into it will lose their parent and become undocked.
			// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
			// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace Demo", p_open, window_flags);
			{
				if (opt_fullscreen)
				{
					ImGui::PopStyleVar(2);
				}

				// DockSpace
				ImGuiIO& io = ImGui::GetIO();
				if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
				{
					ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
					ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
				}
				else
				{
					// ShowDockingDisabledMessage();
				}

				/**** BEGIN Viewport ****/

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::Begin("Viewport");
				auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
				auto viewportSize = ImGui::GetContentRegionAvail();
				ImGui::Image(s_TextureID, viewportSize, { 0, 1 }, { 1, 0 });

				if (s_ViewportWidth != viewportSize.x || s_ViewportHeight != viewportSize.y)
				{
					s_ViewportWidth = (uint32_t)viewportSize.x;
					s_ViewportHeight = (uint32_t)viewportSize.y;
					s_ViewportFBNeedsResize = true;
				}

				Window* mainWindow = Application::Get()->GetWindow();
				UpdateImGuizmo(mainWindow);

				ImGui::End();
				ImGui::PopStyleVar();

				/**** END Viewport ****/

				/**** BEGIN ImGui panels ****/

				// ImGui::Begin("Scene Hierarchy");
				// ImGui::End();

				bool showSceneHierarchyPanel = true;
				VulkanTestLayer::s_SceneHierarchyPanel->OnImGuiRender(&showSceneHierarchyPanel);

				bool showContentBrowserPanel = true;
				// VulkanTestLayer::s_ContentBrowserPanel->OnImGuiRender(&showContentBrowserPanel);

				bool showMaterialEditorPanel = true;
				VulkanTestLayer::s_MaterialEditorPanel->OnImGuiRender(&showMaterialEditorPanel);

				/**** END ImGui panels ****/

				/////////////////////////////////////////////////////////////////////////////////////
				//// ENVIRONMENT
				/////////////////////////////////////////////////////////////////////////////////////

				/**** BEGIN Environment ****/
				ImGui::Begin("Environment");
				{
					if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
					{
						{
							ImGui::Columns(2);

							ImGui::InputText("##envmapfilepath", "", 256, ImGuiInputTextFlags_ReadOnly);

							if (ImGui::BeginDragDropTarget())
							{
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
								{
									std::wstring itemPath = std::wstring((const wchar_t*)payload->Data);
									size_t itemSize = payload->DataSize;
									Log::GetLogger()->debug("END DRAG & DROP FILE '{0}', size: {1}", Util::to_str(itemPath.c_str()).c_str(), itemSize);

									//	m_EnvMapFilename = std::string{ itemPath.begin(), itemPath.end() };
									//	if (m_EnvMapFilename != "")
									//	{
									//		EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(m_EnvMapFilename));
									//	}
								}
								ImGui::EndDragDropTarget();
							}

							ImGui::NextColumn();

							if (ImGui::Button("Load Environment Map"))
							{
								//	m_EnvMapFilename = Application::Get()->OpenFile("*.hdr");
								//	if (m_EnvMapFilename != "")
								//	{
								//		EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(m_EnvMapFilename));
								//	}
							}

							ImGui::NextColumn();

							ImGui::AlignTextToFramePadding();

							if (ImGuiWrapper::Property("Skybox LOD", s_Data.SceneData.SkyboxLod, 0.01f, 0.0f, 4.0f, PropertyFlag::DragProperty))
							{
								// SetSkyboxLOD(skyboxLOD);
							}

							Hazel::HazelLight light; // = EnvMapSceneRenderer::GetActiveLight();
							Hazel::HazelLight lightPrev = light;

							ImGuiWrapper::Property("Light Direction", s_Data.SceneData.LightDirectionTemp, 0.01f, -1.0f, 1.0f, PropertyFlag::DragProperty);
							ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
							ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
							ImGuiWrapper::Property("Exposure", s_Data.Exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);

							float radiancePrefilter = 1.0f; // EnvMapSharedData::s_RadiancePrefilter
							ImGuiWrapper::Property("Radiance Prefiltering", radiancePrefilter);
							float envMapRotation = 0.0f; // EnvMapSharedData::s_EnvMapRotation;
							ImGuiWrapper::Property("Env Map Rotation", envMapRotation, 1.0f, -360.0f, 360.0f, PropertyFlag::DragProperty);

							ImGui::Columns(1);
						}

						ImGui::Separator();

						{
							ImGui::Text("Mesh");

							// Ref<Hazel::Entity> meshEntity;
							std::string meshFullPath = "None";

							std::string fileName = Util::GetFileNameFromFullPath(meshFullPath);
							ImGui::Text(fileName.c_str()); ImGui::SameLine();
							if (ImGui::Button("...##Mesh"))
							{
								std::string fullPath = Application::Get()->OpenFile();
								if (fullPath != "")
								{
									// Hazel::Entity entity = LoadEntity(fullPath);
								}
							}

							//	auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
							//	if (meshEntities.size())
							//	{
							//		meshEntity = GetMeshEntity();
							//		auto& meshComponent = meshEntity->GetComponent<Hazel::MeshComponent>();
							//		if (meshComponent.Mesh) {
							//			ImGui::SameLine();
							//			ImGui::Checkbox("Is Animated", &meshComponent.Mesh->IsAnimated());
							//		}
							//	}
						}
					}
				}
				ImGui::End();
				/**** END Environment ****/

				/**** BEGIN DockSpace menu bar ****/

				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Docking"))
					{
						// Disabling fullscreen would allow the window to be moved to the front of other windows,
						// which we can't undo at the moment without finer window depth/z control.
						//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

						// if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
						// if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
						// if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^=  ImGuiDockNodeFlags_NoDockingInCentralNode;
						// if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
						// if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
						ImGui::Separator();
						if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
							*p_open = false;
						ImGui::EndMenu();
					}
					ImGui::EndMenuBar();
				}
			}
			ImGui::End(); // END DockSpace Demo
			ImGui::PopStyleVar();

			/**** END DockSpace menu bar ****/

			// TODO: Move to VulkanImGuiLayer
			// Rendering
			ImGui::Render();

			ImDrawData* main_draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(main_draw_data, s_ImGuiCommandBuffer);

			VK_CHECK_RESULT(vkEndCommandBuffer(s_ImGuiCommandBuffer));

			commandBuffers.push_back(s_ImGuiCommandBuffer);
		}
	}

	// TODO: Temporary method until composite rendering is enabled
	void VulkanRenderer::Draw(HazelCamera* camera)
	{
		s_Data.SceneData.SceneCamera.Camera = *camera;

		if (s_ViewportFBNeedsResize)
		{
			s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
			s_ViewportFBNeedsResize = false;
		}

		GeometryPass();
		CompositePass();
	}

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> VulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		const uint32_t cubemapSize = 1024;
		const uint32_t irradianceMapSize = 32;

		if (!s_Data.envUnfiltered)
		{
			s_Data.envUnfiltered = HazelTextureCube::Create(HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);
		}

		s_Data.envEquirect = HazelTexture2D::Create(filepath);
		HazelImageFormat envEquirectImageFormat = s_Data.envEquirect->GetFormat();
		/****
		HZ_CORE_ASSERT(s_Data.envEquirect->GetFormat() == HazelImageFormat::RGBA16F, "Texture is not HDR!");
		if (envEquirectImageFormat != HazelImageFormat::RGBA16F)
		{
			Log::GetLogger()->error("Texture '{0}' is not HDR (format: '{1}')!", filepath, envEquirectImageFormat);
			return std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>>();
		}
		****/

		// Convert equirectangular to cubemap
		Ref<HazelShader> equirectangularConversionShader = HazelRenderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		Ref<VulkanComputePipeline> equirectangularConversionPipeline = Ref<VulkanComputePipeline>::Create(equirectangularConversionShader);

		// HazelRenderer::Submit([equirectangularConversionPipeline, envUnfiltered, envEquirect, cubemapSize]() mutable {});
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = equirectangularConversionPipeline->GetShader();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			VulkanShader::ShaderMaterialDescriptorSet descriptorSet = shader->CreateDescriptorSets();

			Ref<VulkanTextureCube> envUnfilteredCubemap = s_Data.envUnfiltered.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_CubeMap");
			writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
			writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
			writeDescriptors[0].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();

			Ref<VulkanTexture2D> envEquirectVK = s_Data.envEquirect.As<VulkanTexture2D>();
			writeDescriptors[1] = *shader->GetDescriptorSet("u_EquirectangularTex");
			writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data(); // Should this be set inside the shader?
			writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
			writeDescriptors[1].pImageInfo = &envEquirectVK->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
			equirectangularConversionPipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), cubemapSize / 32, cubemapSize / 32, 6);

			VkQueue computeQueue = VulkanContext::GetCurrentDevice()->GetComputeQueue();
			vkQueueWaitIdle(computeQueue);

			envUnfilteredCubemap->GenerateMips(true);
		}

		// MipFiltering
		Ref<HazelShader> environmentMipFilterShader = HazelRenderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
		Ref<VulkanComputePipeline> environmentMipFilterPipeline = Ref<VulkanComputePipeline>::Create(environmentMipFilterShader);

		if (!s_Data.envFiltered)
		{
			s_Data.envFiltered = HazelTextureCube::Create(HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);
		}

		// HazelRenderer::Submit([environmentMipFilterPipeline, cubemapSize, envFiltered, envUnfiltered]() mutable {});
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentMipFilterPipeline->GetShader();

			Ref<VulkanTextureCube> envFilteredCubemap = s_Data.envFiltered.As<VulkanTextureCube>();
			VkDescriptorImageInfo imageInfo = envFilteredCubemap->GetVulkanDescriptorInfo();

			uint32_t totalMipLevels = s_MipMapsEnabled ? 11 : 1;

			std::vector<VkWriteDescriptorSet> writeDescriptors;
			std::vector<VkDescriptorImageInfo> mipImageInfos;

			writeDescriptors.resize(totalMipLevels * 2);
			mipImageInfos.resize(totalMipLevels);

			VulkanShader::ShaderMaterialDescriptorSet descriptorSet = shader->CreateDescriptorSets(0, totalMipLevels);

			for (uint32_t i = 0; i < totalMipLevels; i++)
			{
				VkDescriptorImageInfo& mipImageInfo = mipImageInfos[i];
				mipImageInfo = imageInfo;
				mipImageInfo.imageView = envFilteredCubemap->CreateImageViewSingleMip(i);
				mipImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				writeDescriptors[i * 2 + 0] = *shader->GetDescriptorSet("outputTexture");
				writeDescriptors[i * 2 + 0].dstSet = descriptorSet.DescriptorSets[i];
				writeDescriptors[i * 2 + 0].pImageInfo = &mipImageInfo;

				Ref<VulkanTextureCube> envUnfilteredCubemap = s_Data.envUnfiltered.As<VulkanTextureCube>();
				writeDescriptors[i * 2 + 1] = *shader->GetDescriptorSet("inputTexture");
				writeDescriptors[i * 2 + 1].dstSet = descriptorSet.DescriptorSets[i];
				writeDescriptors[i * 2 + 1].pImageInfo = &envUnfilteredCubemap->GetVulkanDescriptorInfo();
			}

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);

			environmentMipFilterPipeline->Begin(); // begin compute pass
			const float deltaRoughness = 1.0f / glm::max((float)s_Data.envFiltered->GetMipLevelCount() - 1.0f, 1.0f);
			for (uint32_t i = 0, size = cubemapSize; i < totalMipLevels; i++, size /= 2)
			{
				uint32_t numGroups = glm::max(1u, size / 32);
				float roughness = i * deltaRoughness;
				roughness = glm::max(roughness, 0.05f);
				environmentMipFilterPipeline->SetPushConstants(&roughness, sizeof(float));
				environmentMipFilterPipeline->Dispatch(descriptorSet.DescriptorSets[i], numGroups, numGroups, 6);
			}
			environmentMipFilterPipeline->End();

			VkQueue computeQueue = VulkanContext::GetCurrentDevice()->GetComputeQueue();
			vkQueueWaitIdle(computeQueue);
		}

		// Irradiance map
		Ref<HazelShader> environmentIrradianceShader = HazelRenderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
		Ref<VulkanComputePipeline> environmentIrradiancePipeline = Ref<VulkanComputePipeline>::Create(environmentIrradianceShader);


		if (!s_Data.irradianceMap)
		{
			// s_Data.irradianceMap = HazelTextureCube::Create(HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);
			s_Data.irradianceMap = HazelTextureCube::Create(HazelImageFormat::RGBA16F, irradianceMapSize, irradianceMapSize);
		}

		// HazelRenderer::Submit([environmentIrradiancePipeline, envFilteredCubemap, s_Data.irradianceMap, irradianceMapSize]() mutable {});
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			Ref<VulkanShader> shader = environmentIrradiancePipeline->GetShader();

			std::array<VkWriteDescriptorSet, 2> writeDescriptors;
			VulkanShader::ShaderMaterialDescriptorSet descriptorSet = shader->CreateDescriptorSets();

			Ref<VulkanTextureCube> irradianceCubemap = s_Data.irradianceMap.As<VulkanTextureCube>();
			writeDescriptors[0] = *shader->GetDescriptorSet("o_IrradianceMap");
			writeDescriptors[0].dstSet = *descriptorSet.DescriptorSets.data();
			writeDescriptors[0].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
			writeDescriptors[0].pImageInfo = &irradianceCubemap->GetVulkanDescriptorInfo();

			Ref<VulkanTextureCube> envFilteredCubemap = s_Data.envFiltered.As<VulkanTextureCube>();
			writeDescriptors[1] = *shader->GetDescriptorSet("u_RadianceMap");
			writeDescriptors[1].dstSet = *descriptorSet.DescriptorSets.data();
			writeDescriptors[1].descriptorCount = (uint32_t)descriptorSet.DescriptorSets.size();
			writeDescriptors[1].pImageInfo = &envFilteredCubemap->GetVulkanDescriptorInfo();

			vkUpdateDescriptorSets(device, (uint32_t)writeDescriptors.size(), writeDescriptors.data(), 0, nullptr);
			environmentIrradiancePipeline->Execute(descriptorSet.DescriptorSets.data(), (uint32_t)descriptorSet.DescriptorSets.size(), irradianceCubemap->GetWidth() / 32, irradianceCubemap->GetHeight() / 32, 6);

			VkQueue computeQueue = VulkanContext::GetCurrentDevice()->GetComputeQueue();
			vkQueueWaitIdle(computeQueue);

			irradianceCubemap->GenerateMips(true);
		}

		return { s_Data.envFiltered, s_Data.irradianceMap };
	}

	void VulkanRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
	}

	void VulkanRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		Log::GetLogger()->error("The virtual method RenderMesh currently not in use. Use RenderMeshStatic instead!");
	}

	void VulkanRenderer::RenderMeshStatic(Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
		// HazelRenderer::Submit([mesh, transform]() mutable {});
		{
			Ref<VulkanPipeline> vulkanPipeline = s_MeshPipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = mesh->GetVertexBuffer().As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Ref<VulkanIndexBuffer>(mesh->GetIndexBuffer());
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			auto& submeshes = mesh->GetSubmeshes();
			for (Submesh& submesh : submeshes)
			{
				auto& material = mesh->GetMaterials()[submesh.MaterialIndex].As<VulkanMaterial>();
				material->UpdateForRendering();
				Buffer uniformStorageBuffer = material->GetUniformStorageBuffer();

				VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				// Bind descriptor sets describing shader binding points
				std::array<VkDescriptorSet, 2> descriptorSets = {
					mesh->GetDescriptorSet(submesh.MaterialIndex).DescriptorSet.DescriptorSets[0],
					s_Data.RendererDescriptorSetFeb2021.DescriptorSets[0],
				};
				vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);

				glm::mat4 worldTransform = transform * submesh.Transform;

				vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &worldTransform);
				vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size - 64, &uniformStorageBuffer.Data + 64);
				vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
			}
		}
	}

	void VulkanRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		Log::GetLogger()->error("The virtual method RenderQuad currently not in use. Use RenderQuadStatic instead!");
	}

	void VulkanRenderer::RenderQuadStatic(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		vulkanMaterial->UpdateForRendering(); // Broken at the moment

		// HazelRenderer::Submit([pipeline, vulkanMaterial, transform]() {});
		{
			Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

			// VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			// vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			// vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			// VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			// vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			// vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &vulkanMaterial->GetDescriptorSet().DescriptorSets[0], 0, nullptr);

			// Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

			// vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
			// vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			// vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, s_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		}
	}

	void VulkanRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
	{
		Log::GetLogger()->error("The virtual method SubmitFullscreenQuad currently not in use. Use SubmitFullscreenQuadStatic instead!");
	}

	// TODO: virtual or static?
	void VulkanRenderer::SubmitFullscreenQuadStatic(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
	{
		Ref<VulkanMaterial> vulkanMaterial = material.As<VulkanMaterial>();
		vulkanMaterial->UpdateForRendering();

		// HazelRenderer::Submit([pipeline, vulkanMaterial]() mutable {});
		{
			Ref<VulkanPipeline> vulkanPipeline = s_CompositePipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data.ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data.ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			// vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)s_QuadDescriptorSet.DescriptorSets.size(), s_QuadDescriptorSet.DescriptorSets.data(), 0, nullptr);
			vkCmdBindDescriptorSets(s_Data.ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, (uint32_t)vulkanMaterial->GetDescriptorSet().DescriptorSets.size(), vulkanMaterial->GetDescriptorSet().DescriptorSets.data(), 0, nullptr);

			Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

			vkCmdPushConstants(s_Data.ActiveCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);
			vkCmdDrawIndexed(s_Data.ActiveCommandBuffer, s_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		}
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data.RenderCaps;
	}

	void VulkanRenderer::UpdateImGuizmo(Window* mainWindow)
	{
		// BEGIN ImGuizmo

		// ImGizmo switching modes
		if (Input::IsKeyPressed(Key::D1))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

		if (Input::IsKeyPressed(Key::D2))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

		if (Input::IsKeyPressed(Key::D3))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

		if (Input::IsKeyPressed(Key::D4))
			Scene::s_ImGuizmoType = -1;

		// ImGuizmo
		if (Scene::s_ImGuizmoType != -1)
		{
			float rw = (float)ImGui::GetWindowWidth();
			float rh = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

			if (s_SelectedSubmesh != nullptr) {
				s_Transform_ImGuizmo = &s_SelectedSubmesh->Transform; // Connect to model transform
			}

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 1.0f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE) {
				snapValue = 45.0f;
			}

			float snapValues[3] = { snapValue, snapValue, snapValue };

			if (s_Transform_ImGuizmo != nullptr) // TODO: specify display criteria here
			{
				ImGuizmo::Manipulate(
					glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix()),
					glm::value_ptr(s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix()),
					(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
					ImGuizmo::WORLD,
					glm::value_ptr(*s_Transform_ImGuizmo),
					nullptr,
					snap ? snapValues : nullptr);
			}
		}
		// END ImGuizmo
	}

	uint32_t VulkanRenderer::GetViewportWidth()
	{
		return s_ViewportWidth;
	}

	uint32_t VulkanRenderer::GetViewportHeight()
	{
		return s_ViewportHeight;
	}

	int32_t& VulkanRenderer::GetSelectedDrawCall()
	{
		int32_t v;
		return v; // TODO: s_Data.SelectedDrawCall;
	}

	glm::vec3 VulkanRenderer::GetLightDirectionTemp()
	{
		return s_Data.SceneData.LightDirectionTemp;
	}

}
