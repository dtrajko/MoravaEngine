#include "VulkanTestLayer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanSwapChain.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

#include "Core/Application.h"
#include "HazelVulkan/ExampleVertex.h"


namespace Hazel {

	VulkanTestLayer::VulkanTestLayer()
		: m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	VulkanTestLayer::VulkanTestLayer(const std::string& name)
		: Layer(name), m_Camera(glm::perspectiveFov(glm::radians(45.0f), 1280.0f, 720.0f, 0.1f, 1000.0f))
	{
	}

	VulkanTestLayer::~VulkanTestLayer()
	{
	}

	void VulkanTestLayer::OnAttach()
	{
		m_Meshes.push_back(Ref<HazelMesh>::Create("Models/Cerberus/CerberusMaterials.fbx"));
		// m_Meshes.push_back(Ref<HazelMesh>::Create("Models/Gladiator/Gladiator.fbx"));
	}

	void VulkanTestLayer::OnDetach()
	{
	}

	void VulkanTestLayer::OnUpdate(Timestep ts)
	{
		m_Camera.SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), (float)VulkanRenderer::GetViewportWidth(), (float)VulkanRenderer::GetViewportHeight(), 0.01f, 1000.0f));

		m_Camera.OnUpdate(ts);

		glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		Render(clearColor, m_Camera);
		for (Ref<HazelMesh> mesh : m_Meshes)
		{
			VulkanRenderer::SubmitMesh(mesh);
		}
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

	void VulkanTestLayer::Render(const glm::vec4& clearColor, const EditorCamera& camera)
	{
		auto mesh = m_Meshes[0];
		// HazelRenderer::Submit([=]() mutable
		// {
		// });
		{
			Ref<VulkanContext> context = Ref<VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
			Ref<VulkanShader> shader = mesh->GetMeshShader().As<VulkanShader>();
			VulkanSwapChain& swapChain = context->GetSwapChain();

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
				// uniform buffer binding 0 uniform Camera
				void* ubPtr = shader->MapUniformBuffer(0);
				glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)swapChain.GetWidth(), (float)swapChain.GetHeight(), 0.1f, 1000.0f);
				// glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 4.0f)));
				glm::mat4 viewProj = proj * camera.GetViewMatrix();  // Runtime camera
				// glm::mat4 viewProj = m_Camera.GetViewProjection(); // Editor camera
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

				// VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();

				// DRAW GEO HERE

				vkCmdEndRenderPass(drawCommandBuffer);

				// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
				// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

				VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
			}
#endif
		}
	}
}
