#include "VulkanTestLayer.h"


VulkanTestLayer::VulkanTestLayer()
{
}

VulkanTestLayer::~VulkanTestLayer()
{
}

void VulkanTestLayer::OnAttach()
{
}

void VulkanTestLayer::OnDetach()
{
}

void VulkanTestLayer::OnUpdate(Hazel::Timestep ts)
{
	static glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 0.0f };
	static float delta = 0.0002f;
	if (clearColor.r > 1.0f || clearColor.r < 0.0f) {
		delta = -delta;
	}

	clearColor.r += delta;
	clearColor.b += delta;

	BuildCommandBuffer(clearColor);
}

void VulkanTestLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
}

void VulkanTestLayer::OnEvent(Event& event)
{
}

void VulkanTestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
}

void VulkanTestLayer::OnRender(Window* mainWindow)
{
	RendererBasic::Clear(1.0f, 0.0f, 1.0f, 1.0f);

}

void VulkanTestLayer::BuildCommandBuffer(const glm::vec4& clearColor)
{
	/****
	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	// Set clear values for all framebuffer attachments with loadOp set to clear
	// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
	VkClearValue clearValues[2];
	clearValues[0].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
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
		void* ubPtr = shader->MapUniformBuffer(0);
		glm::mat4 viewProj = camera.GetViewProjection();
		memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
		shader->UnmapUniformBuffer(0);
	}

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

		VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

		auto vulkanMeshVB = mesh->GetVertexBuffer().As<VulkanVertexBuffer>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(drawCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = Ref<VulkanIndexBuffer>(mesh->GetIndexBuffer());
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(drawCommandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		auto& submeshes = mesh->GetSubmeshes();
		for (Submesh& submesh : submeshes)
		{
			VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
			vkCmdBindPipeline(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Bind descriptor sets describing shader binding points
			VkDescriptorSet* descriptorSet = (VkDescriptorSet*)m_Mesh->GetDescriptorSet();
			vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, descriptorSet, 0, nullptr);

			vkCmdPushConstants(drawCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submesh.Transform);
			glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			vkCmdPushConstants(drawCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);
			vkCmdDrawIndexed(drawCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
		}

		vkCmdEndRenderPass(drawCommandBuffer);

		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
	}
	****/
}