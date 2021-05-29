#include "VulkanTestLayer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanSwapChain.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

#include "Core/Application.h"
#include "HazelVulkan/ExampleVertex.h"


VulkanTestLayer::VulkanTestLayer()
	: m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
{

}

VulkanTestLayer::~VulkanTestLayer()
{
}

void VulkanTestLayer::OnAttach()
{
	m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx");

	Hazel::HazelFramebufferSpecification spec;
}

void VulkanTestLayer::OnDetach()
{
}

// void VulkanTestLayer::OnUpdate(Hazel::Timestep ts, Hazel::HazelCamera* camera) // const Hazel::EditorCamera& camera
// {
// 	m_Camera.OnUpdate(ts);
// 
// 	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
// 	Render(clearColor, camera);
// }

void VulkanTestLayer::OnUpdate(Hazel::Timestep ts)
{
	m_Camera.OnUpdate(ts);

	static glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	Render(clearColor, m_Camera);
	Hazel::VulkanRenderer::SubmitMesh(m_Mesh);
}

void VulkanTestLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
}

void VulkanTestLayer::OnEvent(Event& event)
{
	m_Camera.OnEvent(event);

	if (event.GetEventType() == EventType::WindowResize)
	{
		WindowResizeEvent& e = (WindowResizeEvent&)event;
		if (e.GetWidth() != 0 && e.GetHeight() != 0)
		{
			m_Camera.SetViewportSize((float)e.GetWidth(), (float)e.GetHeight());
			m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), (float)e.GetWidth(), (float)e.GetHeight(), 0.1f, 10000.0f));
		}
	}
}

void VulkanTestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
}

void VulkanTestLayer::OnRender(Window* mainWindow)
{
	// RendererBasic::Clear(1.0f, 0.0f, 1.0f, 1.0f);
}

void VulkanTestLayer::Render(const glm::vec4& clearColor, const Hazel::EditorCamera& camera) // const Hazel::EditorCamera& camera
{
	// Hazel::HazelRenderer::Submit([=]() mutable
	// {
	// });
	{
		Hazel::Ref<Hazel::VulkanContext> context = Hazel::Ref<Hazel::VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
		Hazel::Ref<Hazel::VulkanPipeline> vulkanPipeline = m_Mesh->GetPipeline().As<Hazel::VulkanPipeline>();
		Hazel::Ref<Hazel::VulkanShader> shader = vulkanPipeline->GetSpecification().Shader.As<Hazel::VulkanShader>();
		Hazel::VulkanSwapChain& swapChain = context->GetSwapChain();

#if 0
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

#endif

		{
			void* ubPtr = shader->MapUniformBuffer(0);
			glm::mat4 viewProj = camera.GetViewProjection();
			memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
			shader->UnmapUniformBuffer(0);
		}

#if 0
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

			// DRAW GEO HERE

			/**** BEGIN mesh geometry ****/
			{
				auto vulkanMeshVB = m_Mesh->GetVertexBuffer().As<Hazel::VulkanVertexBuffer>();
				VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindVertexBuffers(drawCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

				auto vulkanMeshIB = Hazel::Ref<Hazel::VulkanIndexBuffer>(m_Mesh->GetIndexBuffer());
				VkBuffer ibMeshBuffer = vulkanMeshIB->GetVulkanBuffer();
				vkCmdBindIndexBuffer(drawCommandBuffer, ibMeshBuffer, 0, VK_INDEX_TYPE_UINT32);

				auto& submeshes = m_Mesh->GetSubmeshes();
				for (Hazel::Submesh& submesh : submeshes)
				{
					VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
					vkCmdBindPipeline(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

					// Bind descriptor sets describing shader binding points
					VkDescriptorSet descriptorSet = m_Mesh->GetDescriptorSet();
					vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

					// Push Constants
					vkCmdPushConstants(drawCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submesh.Transform);

					glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
					vkCmdPushConstants(drawCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &color);

					vkCmdDrawIndexed(drawCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
				}
			}

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
#endif
	}
}
