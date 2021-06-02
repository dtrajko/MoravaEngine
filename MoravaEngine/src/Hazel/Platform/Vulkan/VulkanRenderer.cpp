#include "VulkanRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanFramebuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan_with_textures.h"

#include "../../ImGuizmo/ImGuizmo.h"


namespace Hazel {

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

	void VulkanRenderer::SubmitMesh(const Ref<HazelMesh>& mesh)
	{
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

	void VulkanRenderer::Init()
	{
		// HazelRenderer::Submit([=]() {
		// });
		{
			s_ImGuiCommandBuffer = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
			s_CompositeCommandBuffer = VulkanContext::GetCurrentDevice()->CreateSecondaryCommandBuffer();
		}

		{
			HazelFramebufferSpecification spec;
			spec.Width = s_ViewportWidth;
			spec.Height = s_ViewportHeight;
			s_Framebuffer = HazelFramebuffer::Create(spec);
			s_Framebuffer->AddResizeCallback([](Ref<HazelFramebuffer> framebuffer) {
				// HazelRenderer::Submit([framebuffer]() mutable
				// {
				// });
				{
					auto vulkanFB = framebuffer.As<VulkanFramebuffer>();
					const auto& imageInfo = vulkanFB->GetVulkanDescriptorInfo();
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
			pipelineSpecification.Shader = HazelRenderer::GetShaderLibrary()->Get("HazelPBR_Static");

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
			pipelineSpecification.Shader = HazelRenderer::GetShaderLibrary()->Get("Texture");

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
	}

	static void RenderMesh(Ref<HazelMesh> mesh, VkCommandBuffer commandBuffer, HazelCamera* camera) // TODO: remove the HazelCamera parameter
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
			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			VkDescriptorSet* descriptorSet = (VkDescriptorSet*)mesh->GetDescriptorSet();
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, descriptorSet, 0, nullptr);

			// Push Constants
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submesh.Transform);

			glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);

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
				RenderMesh(mesh, drawCommandBuffer, camera);
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

#if 0
			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			// Set clear values for all framebuffer attachments with loadOp set to clear
			// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
			VkClearValue clearValues[2];
			clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
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
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;

			// Set target frame buffer
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			{
				VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
				VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));

				// Start the first sub pass specified in our default render pass setup by the base class
				// This will clear the color and depth attachment
				vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Update dynamic viewport state
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = (float)height;
				viewport.height = -(float)height;
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

				// DRAW GEO HERE

				/**** BEGIN rendering meshes ****/
				for (auto& mesh : s_Meshes)
				{
					RenderMesh(mesh, drawCommandBuffer, camera);
				}

				s_Meshes.clear();
				/**** END rendering meshes ****/

				/**** BEGIN ImGui render ****/

				ImGui::Begin("Viewport");
				ImGui::Button("Hello");
				ImGui::End();

				// TODO: Move to VulkanImGuiLayer
				// Rendering
				ImGui::Render();

				// ImGui record commands to command buffer
				ImDrawData* main_draw_data = ImGui::GetDrawData();
				ImGui_ImplVulkan_RenderDrawData(main_draw_data, drawCommandBuffer); // 3rd optional param vulkanPipeline->GetVulkanPipeline()
				/**** END ImGui render ****/

				vkCmdEndRenderPass(drawCommandBuffer);

				// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
				// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

				VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
			}
#endif

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
					bool p_open = true;
					ShowExampleAppDockSpace(&p_open);

					ImGui::Begin("Scene Hierarchy");
					ImGui::End();

					// TEMP: Render Viewport
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
					ImGui::Begin("Viewport");
					auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
					auto viewportSize = ImGui::GetContentRegionAvail();
					ImGui::Image(s_TextureID, viewportSize, { 0, 1 }, { 1, 0 });

					if (s_ViewportWidth != viewportSize.x || s_ViewportHeight != viewportSize.y)
					{
						s_ViewportWidth = (uint32_t)viewportSize.x;
						s_ViewportHeight = (uint32_t)viewportSize.y;
						// s_Framebuffer->Resize(s_ViewportWidth, s_ViewportHeight, true);
					}

					Window* mainWindow = Application::Get()->GetWindow();
					UpdateImGuizmo(mainWindow);

					ImGui::End();
					ImGui::PopStyleVar();

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

#if 0
	void VulkanRenderer::DrawOld(HazelCamera* camera)
	{
		// HazelRenderer::Submit([=]() mutable
		// {
		// });
		{
			Ref<VulkanContext> context = Ref<VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
			// Ref<VulkanPipeline> vulkanPipeline = mesh->GetPipeline().As<VulkanPipeline>();
			// Ref<VulkanShader> shader = vulkanPipeline->GetSpecification().Shader.As<VulkanShader>();
			VulkanSwapChain& swapChain = context->GetSwapChain();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;

			// Set clear values for all framebuffer attachments with loadOp set to clear
			// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
			VkClearValue clearValues[2];
			clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
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
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;

			// Set target frame buffer
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			{
				VkCommandBuffer drawCommandBuffer = swapChain.GetCurrentDrawCommandBuffer();
				VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffer, &cmdBufInfo));

				// Start the first sub pass specified in our default render pass setup by the base class
				// This will clear the color and depth attachment
				vkCmdBeginRenderPass(drawCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Update dynamic viewport state
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = (float)height;
				viewport.height = -(float)height;
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

				// DRAW GEO HERE

				/**** BEGIN rendering meshes ****/
				for (auto& mesh : s_Meshes)
				{
					RenderMesh(mesh, drawCommandBuffer, camera);
				}

				s_Meshes.clear();
				/**** END rendering meshes ****/

				/**** BEGIN ImGui render ****/
				// TODO: Move to VulkanImGuiLayer
				// Rendering
				ImGui::Render();

				// ImGui record commands to command buffer
				ImDrawData* main_draw_data = ImGui::GetDrawData();
				ImGui_ImplVulkan_RenderDrawData(main_draw_data, drawCommandBuffer); // 3rd optional param vulkanPipeline->GetVulkanPipeline()
				/**** END ImGui render ****/

				vkCmdEndRenderPass(drawCommandBuffer);

				// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
				// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

				VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
			}
		}
	}
#endif

	//-----------------------------------------------------------------------------
	// [SECTION] Example App: Docking, DockSpace / ShowExampleAppDockSpace()
	//-----------------------------------------------------------------------------

	// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
	// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows
	// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
	// DockSpace() is only useful to construct to a central location for your application.
	void VulkanRenderer::ShowExampleAppDockSpace(bool* p_open)
	{
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
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

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

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Docking"))
			{
				// Disabling fullscreen would allow the window to be moved to the front of other windows,
				// which we can't undo at the moment without finer window depth/z control.
				//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

				if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
				if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
				if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
				if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
				if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
				ImGui::Separator();
				if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
					*p_open = false;
				ImGui::EndMenu();
			}
			/****
			HelpMarker(
				"When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
				" > if io.ConfigDockingWithShift==false (default):" "\n"
				"   drag windows from title bar to dock" "\n"
				" > if io.ConfigDockingWithShift==true:" "\n"
				"   drag windows from anywhere and hold Shift to dock" "\n\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)"
			);
			****/

			ImGui::EndMenuBar();
		}

		ImGui::End();
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

			// Entity transform
			glm::mat4 entityTransform = glm::mat4(1.0f); // Connect to model transform

			// Snapping
			bool snap = Input::IsKeyPressed(Key::LeftControl);
			float snapValue = 1.0f; // Snap to 0.5m for translation/scale
			// Snap to 45 degrees for rotation
			if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE) {
				snapValue = 45.0f;
			}

			float snapValues[3] = { snapValue, snapValue, snapValue };

			if (true) // TODO: specify display criteria here
			{
				ImGuizmo::Manipulate(
					glm::value_ptr(glm::mat4(1.0f)),
					glm::value_ptr(glm::mat4(1.0f)),
					(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
					ImGuizmo::LOCAL,
					glm::value_ptr(entityTransform),
					nullptr,
					snap ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 translation, rotation, scale;
					Math::DecomposeTransform(entityTransform, translation, rotation, scale);

					glm::vec3 deltaRotation = glm::vec3(0.0f); // TODO: add rotation delta here
				}
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

}
