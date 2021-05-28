#include "VulkanRenderer.h"

#include "imgui.h"

#include "Vulkan.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanPipeline.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"

#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"


namespace Hazel {

	void VulkanRenderer::Draw()
	{
		// HazelRenderer::Submit([=]() mutable
		// {
		// });
		{
			Ref<VulkanContext> context = Ref<VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
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

				/**** BEGIN mesh geometry ****/

				// TODO: Move to VulkanImGuiLayer
				// Rendering
				ImGui::Render();

				// ImGui record commands to command buffer
				ImDrawData* main_draw_data = ImGui::GetDrawData();
				ImGui_ImplVulkan_RenderDrawData(main_draw_data, drawCommandBuffer); // 3rd optional param vulkanPipeline->GetVulkanPipeline()

				vkCmdEndRenderPass(drawCommandBuffer);

				// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
				// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

				VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
			}
		}
	}
}
