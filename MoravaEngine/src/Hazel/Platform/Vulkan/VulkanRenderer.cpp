#include "VulkanRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
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

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan_with_textures.h"

#include "ImGuizmo.h"


namespace Hazel {

	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;

		Ref<HazelTexture2D> BRDFLut;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		VulkanShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

		std::unordered_map<SceneRenderer*, std::vector<VulkanShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
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

		// VulkanRendererDataOld
		VkCommandBuffer ActiveCommandBuffer = nullptr;
		Ref<HazelShaderLibrary> m_ShaderLibrary;
	};

	static VulkanRendererData* s_Data = nullptr;

	static VkCommandBuffer s_ImGuiCommandBuffer;
	static VkCommandBuffer s_CompositeCommandBuffer;

	static Ref<HazelFramebuffer> s_Framebuffer;
	static Ref<Pipeline> s_MeshPipeline;
	static Ref<Pipeline> s_CompositePipeline;
	static Ref<VertexBuffer> s_QuadVertexBuffer;
	static Ref<IndexBuffer> s_QuadIndexBuffer;
	static VkDescriptorSet s_QuadDescriptorSet;
	static ImTextureID s_TextureID;
	static uint32_t s_ViewportWidth = 1280;
	static uint32_t s_ViewportHeight = 720;

	static std::vector<Ref<HazelMesh>> s_Meshes;

	static Submesh* s_SelectedSubmesh;
	static glm::mat4* s_Transform_ImGuizmo = nullptr;

	namespace Utils {

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

	void VulkanRenderer::Init()
	{
		s_Data = new VulkanRendererData();
		auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);

		Utils::DumpGPUInfo();

		// TODO: Create descriptor pools

		/**** BEGIN code from HazelRenderer::Init() ****/

		s_Data->m_ShaderLibrary = Ref<HazelShaderLibrary>::Create();

		// s_Data->m_ShaderLibrary->Load("assets/shaders/Grid.glsl");
		// s_Data->m_ShaderLibrary->Load("assets/shaders/SceneComposite.glsl");
		// s_Data->m_ShaderLibrary->Load("assets/shaders/HazelSimple.glsl");
		s_Data->m_ShaderLibrary->Load("assets/shaders/HazelPBR_Static.glsl");
		// s_Data->m_ShaderLibrary->Load("assets/shaders/Outline.glsl");
		// s_Data->m_ShaderLibrary->Load("assets/shaders/Skybox.glsl");
		s_Data->m_ShaderLibrary->Load("assets/shaders/Texture.glsl");

		SceneRenderer::Init();

		// Renderer2D::Init();

		/**** END code from HazelRenderer::Init() ****/

		// HazelRenderer::Submit([=]() {
		// });
		{
			s_ImGuiCommandBuffer = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
			s_CompositeCommandBuffer = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
		}

		{
			HazelFramebufferSpecification spec;
			spec.DebugName = "Viewport";
			spec.Width = s_ViewportWidth;
			spec.Height = s_ViewportHeight;
			s_Framebuffer = HazelFramebuffer::Create(spec);
			s_Framebuffer->AddResizeCallback([](Ref<HazelFramebuffer> framebuffer)
			{
				// HazelRenderer::Submit([framebuffer]() mutable
				// {
				// });
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
			pipelineSpecification.Shader = s_Data->m_ShaderLibrary->Get("HazelPBR_Static");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = s_Framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);

			s_MeshPipeline = Pipeline::Create(pipelineSpecification);
		}

		{
			HazelFramebufferSpecification spec;
			spec.SwapChainTarget = true;
			Ref<HazelFramebuffer> framebuffer = HazelFramebuffer::Create(spec);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				// { ShaderDataType::Float3, "a_Normal" },
				// { ShaderDataType::Float3, "a_Tangent" },
				// { ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.Shader = s_Data->m_ShaderLibrary->Get("Texture");

			RenderPassSpecification renderPassSpec;
			renderPassSpec.TargetFramebuffer = framebuffer;
			pipelineSpecification.RenderPass = RenderPass::Create(renderPassSpec);
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

		// HazelRenderer::Submit([=]()
		// {
		// });
		{
			auto shader = s_CompositePipeline->GetSpecification().Shader.As<VulkanShader>();
			auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>();
			s_QuadDescriptorSet = shader->CreateDescriptorSet();

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = s_QuadDescriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
			writeDescriptorSet.dstBinding = 0;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);

			auto vulkanFB = s_Framebuffer.As<VulkanFramebuffer>();
			const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
			s_TextureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
		}

		Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
	}

	void VulkanRenderer::Shutdown()
	{
		VulkanShader::ClearUniformBuffers();
		delete s_Data;
	}

	void VulkanRenderer::SubmitMesh(const Ref<HazelMesh>& mesh)
	{
		// Temporary code - populate selected submesh
		std::vector<Submesh> submeshes = mesh->GetSubmeshes();
		s_SelectedSubmesh = &submeshes.at(0);

		s_Meshes.push_back(mesh);
	}

	void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
	{
		// HazelRenderer::Submit([=]() {
		// });
		{
			auto framebuffer = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>();

			VkWriteDescriptorSet writeDescriptorSet = {};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.dstSet = s_QuadDescriptorSet;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.pImageInfo = &framebuffer->GetVulkanDescriptorInfo();
			writeDescriptorSet.dstBinding = 0;

			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			vkUpdateDescriptorSets(vulkanDevice, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	static void RenderMeshVulkan(Ref<HazelMesh> mesh, VkCommandBuffer commandBuffer, HazelCamera* camera) // TODO: remove the HazelCamera parameter
	{
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
			VkDescriptorSet* descriptorSet = (VkDescriptorSet*)mesh->GetDescriptorSet();
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, descriptorSet, 0, nullptr);

			// Push Constants
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submesh.Transform);

			glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			// vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);

			vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
		}
	}

	static void CompositeRenderPass(VkCommandBufferInheritanceInfo& inheritanceInfo)
	{
		Ref<VulkanContext> context = Ref<VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
		VkCommandBuffer commandBuffer = s_CompositeCommandBuffer;

		VulkanSwapChain& swapChain = context->GetSwapChain();
		uint32_t width = swapChain.GetWidth();
		uint32_t height = swapChain.GetHeight();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
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

		auto vulkanMeshVB = s_QuadVertexBuffer.As<VulkanVertexBuffer>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = s_QuadIndexBuffer.As<VulkanIndexBuffer>();
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind descriptor sets describing shader binding points
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &s_QuadDescriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}

	// TODO: Temporary method until composite rendering is enabled
	void VulkanRenderer::Draw(HazelCamera* camera)
	{
		static bool viewportFBNeedsResize = false;
		if (viewportFBNeedsResize)
		{
			s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight);
			viewportFBNeedsResize = false;
		}

		// HazelRenderer::Submit([=]() mutable
		// {
		// });
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

			for (auto& mesh : s_Meshes)
			{
				RenderMeshVulkan(mesh, drawCommandBuffer, camera);
			}

			s_Meshes.clear();

			vkCmdEndRenderPass(drawCommandBuffer);
		}

		// HazelRenderer::Submit([=]()
		// {
		// });
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();

			VkClearValue clearValues[2];
			clearValues[0].color = { {0.1f, 0.1f,0.1f, 1.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };

			uint32_t width = swapChain.GetWidth();
			uint32_t height = swapChain.GetHeight();

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = swapChain.GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2; // Color + depth
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			{
				vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

				VkCommandBufferInheritanceInfo inheritanceInfo = {};
				inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.renderPass = swapChain.GetRenderPass();
				inheritanceInfo.framebuffer = swapChain.GetCurrentFramebuffer();

				std::vector<VkCommandBuffer> commandBuffers;
				CompositeRenderPass(inheritanceInfo);
				commandBuffers.push_back(s_CompositeCommandBuffer);

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
							viewportFBNeedsResize = true;
						}

						Window* mainWindow = Application::Get()->GetWindow();
						UpdateImGuizmo(mainWindow, camera);

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

				vkCmdExecuteCommands(drawCommandBuffer, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

				vkCmdEndRenderPass(drawCommandBuffer);
			}

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
		}
	}

	void VulkanRenderer::BeginFrame()
	{
		//	HazelRenderer::Submit([]()
		//	{
		//	});
		{
			Ref<VulkanContext> context = VulkanContext::Get();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
			s_Data->ActiveCommandBuffer = drawCommandBuffer;
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));
		}
	}

	void VulkanRenderer::EndFrame()
	{
		//	HazelRenderer::Submit([]()
		//	{
		//	});
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(s_Data->ActiveCommandBuffer));
		}
	}

	void VulkanRenderer::BeginRenderPass(const Ref<RenderPass>& renderPass)
	{
		//	HazelRenderer::Submit([renderPass]()
		//	{
		//	});
		{
			BeginFrame();

			// Ref<VulkanFramebuffer> framebuffer = s_Framebuffer.As<VulkanFramebuffer>();
			auto fb = renderPass->GetSpecification().TargetFramebuffer;
			Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
			const auto& fbSpec = framebuffer->GetSpecification();

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

			// TODO: Does out framebuffer has a depth attachment?
			VkClearValue clearValues[2];
			clearValues[0].color = { { fbSpec.ClearColor.r, fbSpec.ClearColor.g, fbSpec.ClearColor.b, fbSpec.ClearColor.a } };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassBeginInfo.clearValueCount = 2; // Color + depth
			renderPassBeginInfo.pClearValues = clearValues;
			renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

			vkCmdBeginRenderPass(s_Data->ActiveCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(s_Data->ActiveCommandBuffer, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(s_Data->ActiveCommandBuffer, 0, 1, &scissor);
		}
	}

	void VulkanRenderer::EndRenderPass()
	{
		//	HazelRenderer::Submit([]()
		//	{
		//	});
		{
			vkCmdEndRenderPass(s_Data->ActiveCommandBuffer);
			s_Data->ActiveCommandBuffer = nullptr;
		}
	}

	void VulkanRenderer::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material)
	{
		//	HazelRenderer::Submit([]()
		//	{
		//	});
		{
			Ref<VulkanPipeline> vulkanPipeline = s_CompositePipeline.As<VulkanPipeline>();

			VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

			auto vulkanMeshVB = s_QuadVertexBuffer.As<VulkanVertexBuffer>();
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(s_Data->ActiveCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = s_QuadIndexBuffer.As<VulkanIndexBuffer>();
			VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(s_Data->ActiveCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			vkCmdBindDescriptorSets(s_Data->ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &s_QuadDescriptorSet, 0, nullptr);

			vkCmdDrawIndexed(s_Data->ActiveCommandBuffer, s_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		}
	}

	void VulkanRenderer::SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow)
	{
	}

	std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> VulkanRenderer::CreateEnvironmentMap(const std::string& filepath)
	{
		return std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>>();
	}

	void VulkanRenderer::RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
	}

	void VulkanRenderer::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform)
	{
	}

	void VulkanRenderer::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
	}

	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}

	//-----------------------------------------------------------------------------
	// [SECTION] Example App: Docking, DockSpace / ShowExampleAppDockSpace()
	//-----------------------------------------------------------------------------

	// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
	// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows
	// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
	// DockSpace() is only useful to construct to a central location for your application.
	////	void VulkanRenderer::ShowExampleAppDockSpace(bool* p_open)
	////	{
	////		static bool opt_fullscreen_persistant = true;
	////		bool opt_fullscreen = opt_fullscreen_persistant;
	////		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
	////	
	////		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	////		// because it would be confusing to have two docking targets within each others.
	////		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	////		if (opt_fullscreen)
	////		{
	////			ImGuiViewport* viewport = ImGui::GetMainViewport();
	////			ImGui::SetNextWindowPos(viewport->GetWorkPos());
	////			ImGui::SetNextWindowSize(viewport->GetWorkSize());
	////			ImGui::SetNextWindowViewport(viewport->ID);
	////			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	////			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	////			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	////			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	////		}
	////	
	////		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
	////		// and handle the pass-thru hole, so we ask Begin() to not render a background.
	////		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	////			window_flags |= ImGuiWindowFlags_NoBackground;
	////	
	////		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	////		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
	////		// all active windows docked into it will lose their parent and become undocked.
	////		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
	////		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	////		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	////		ImGui::Begin("DockSpace Demo", p_open, window_flags);
	////		ImGui::PopStyleVar();
	////	
	////		if (opt_fullscreen)
	////			ImGui::PopStyleVar(2);
	////	
	////		// DockSpace
	////		ImGuiIO& io = ImGui::GetIO();
	////		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	////		{
	////			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	////			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	////		}
	////		else
	////		{
	////			// ShowDockingDisabledMessage();
	////		}
	////	
	////		if (ImGui::BeginMenuBar())
	////		{
	////			if (ImGui::BeginMenu("Docking"))
	////			{
	////				// Disabling fullscreen would allow the window to be moved to the front of other windows,
	////				// which we can't undo at the moment without finer window depth/z control.
	////				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);
	////	
	////				if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
	////				if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
	////				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
	////				if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
	////				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
	////				ImGui::Separator();
	////				if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
	////					*p_open = false;
	////				ImGui::EndMenu();
	////			}
	////			/****
	////			HelpMarker(
	////				"When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
	////				" > if io.ConfigDockingWithShift==false (default):" "\n"
	////				"   drag windows from title bar to dock" "\n"
	////				" > if io.ConfigDockingWithShift==true:" "\n"
	////				"   drag windows from anywhere and hold Shift to dock" "\n\n"
	////				"This demo app has nothing to do with it!" "\n\n"
	////				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application w/in dow ///(e.g. with a menu bar)." "\n\n"
	////				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your a/pp	 lication, ///you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
	////				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy wo	  rkaround //i/s /that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)"
	////			);
	////			****/
	////	
	////			ImGui::EndMenuBar();
	////		}
	////	
	////		ImGui::End();
	////	}

	void VulkanRenderer::UpdateImGuizmo(Window* mainWindow, HazelCamera* camera)
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
					glm::value_ptr(camera->GetViewMatrix()),
					glm::value_ptr(camera->GetProjectionMatrix()),
					(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
					ImGuizmo::LOCAL,
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
		return v; // TODO: s_Data->SelectedDrawCall;
	}

}
