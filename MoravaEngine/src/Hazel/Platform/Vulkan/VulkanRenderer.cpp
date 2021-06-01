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
			spec.Width = Application::Get()->GetWindow()->GetWidth();
			spec.Height = Application::Get()->GetWindow()->GetHeight();
			s_Framebuffer = HazelFramebuffer::Create(spec);

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

			auto vulkanFB = s_MeshPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>();
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

			uint32_t width = framebuffer->GetSpecification().Width;
			uint32_t height = framebuffer->GetSpecification().Height;

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

					ImGui::Begin("Viewport");

					ImGui::Image(s_TextureID, { 1280.0f, 720.0f }, { 0, 1 }, { 1, 0 });
					ImGui::End();

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

	// TODO: remove parameters
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
}
