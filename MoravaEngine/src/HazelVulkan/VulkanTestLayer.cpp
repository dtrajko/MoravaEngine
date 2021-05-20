#include "VulkanTestLayer.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanShader.h"
#include "Hazel/Platform/Vulkan/VulkanSwapChain.h"

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
	// Hazel::VulkanShader::s_Texture = Hazel::HazelTexture2D::Create("Textures/texture_checker.png", false, Hazel::HazelTextureWrap::Clamp);
	// Hazel::VulkanShader::s_Texture = Hazel::HazelTexture2D::Create("Textures/default_material_albedo.png", false, Hazel::HazelTextureWrap::Clamp);

	Hazel::VulkanShader::s_TextureAlbedo = Hazel::HazelTexture2D::Create("Models/Cerberus/Textures/Cerberus_A.tga", false, Hazel::HazelTextureWrap::Clamp);
	Hazel::VulkanShader::s_TextureNormal = Hazel::HazelTexture2D::Create("Models/Cerberus/Textures/Cerberus_N.tga", false, Hazel::HazelTextureWrap::Clamp);

	// Shaders
	m_Shader = Hazel::HazelShader::Create("assets/shaders/VulkanWeekMesh.glsl");
	// m_ShaderHazelPBR_Static = Hazel::HazelShader::Create("assets/shaders/VulkanWeekHazelPBR_Static.glsl");

	// Graphics Pipeline
	Hazel::PipelineSpecification pipelineSpecification;
	pipelineSpecification.Shader = m_Shader;
	m_Pipeline = Hazel::Pipeline::Create(pipelineSpecification);

	/**** BEGIN triangle geometry ****/

	Hazel::Vertex vertices[3] = {
		{{ -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
		{{  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }},
		{{  0.0f,  0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }},
	};

	m_VertexBuffer = Hazel::VertexBuffer::Create(vertices, sizeof(vertices));

	uint32_t indices[3] = { 0, 1, 2 };

	m_IndexBuffer = Hazel::IndexBuffer::Create(indices, sizeof(indices));

	/**** END triangle geometry ****/

	/**** BEGIN mesh geometry ****/

	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Hazel/Sphere1m.fbx");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/suzanne.obj");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/PardCode/sphere_hq.obj");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/ThinMatrix/tree.obj");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/ThinMatrix/barrel.obj");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Old_Stove/udmheheqx_LOD0.fbx");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/ShaderBall/shaderBall.fbx");
	m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Cerberus/Cerberus_LP.FBX");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Gladiator/Gladiator.fbx");
	// m_Mesh = Hazel::Ref<Hazel::HazelMesh>::Create("Models/Hazel/TestScene.fbx");

	// m_Texture = Hazel::HazelTexture2D::Create("Textures/texture_checker.png", false, Hazel::HazelTextureWrap::Clamp);

	/**** END mesh geometry ****/

	// Random RGB values
	srand(static_cast<unsigned>(time(0)));
	for (uint32_t i = 0; i < 10; i++)
	{
		glm::vec4 randomColor = glm::vec4{ 1.0f };
		randomColor.r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		randomColor.g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		randomColor.b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		m_RandomColors.push_back(randomColor);
	}
}

void VulkanTestLayer::OnDetach()
{
}

void VulkanTestLayer::OnUpdate(Hazel::Timestep ts, glm::mat4 viewMatrix)
{
	m_Camera.OnUpdate(ts);

	// static glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	// static float delta = 0.5f;
	// if (clearColor.r > 1.0f || clearColor.r < 0.0f) {
	// 	delta = -delta;
	// }
	// 
	// clearColor.r += delta * ts * 0.05f;
	// clearColor.b += delta * ts * 0.05f;

	// Log::GetLogger()->info("VulkanTestLayer::OnRender clearColor[{0}, {1}, {2}, {3}]", clearColor.r, clearColor.g, clearColor.b, clearColor.a);

	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	BuildCommandBuffer(clearColor, viewMatrix);
}

void VulkanTestLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
}

void VulkanTestLayer::OnEvent(Event& event)
{
	m_Camera.OnEvent(event);
}

void VulkanTestLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
}

void VulkanTestLayer::OnRender(Window* mainWindow)
{
	// RendererBasic::Clear(1.0f, 0.0f, 1.0f, 1.0f);
}

void VulkanTestLayer::BuildCommandBuffer(const glm::vec4& clearColor, glm::mat4 viewMatrix)
{
	auto pipeline = m_Pipeline;
	// auto vulkanVB = Hazel::Ref<Hazel::VulkanVertexBuffer>(m_VertexBuffer);
	// auto vulkanIB = Hazel::Ref<Hazel::VulkanIndexBuffer>(m_IndexBuffer);
	auto mesh = m_Mesh;

	Hazel::HazelRenderer::Submit([=]() mutable
	{
	});

	Hazel::Ref<Hazel::VulkanContext> context = Hazel::Ref<Hazel::VulkanContext>(Application::Get()->GetWindow()->GetRenderContext());
	Hazel::Ref<Hazel::VulkanPipeline> vulkanPipeline = Hazel::Ref<Hazel::VulkanPipeline>(pipeline);
	Hazel::Ref<Hazel::VulkanShader> shader = Hazel::Ref<Hazel::VulkanShader>(pipeline->GetSpecification().Shader);
	Hazel::VulkanSwapChain& swapChain = context->GetSwapChain();

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	// Set clear values for all framebuffer attachments with loadOp set to clear
	// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
	VkClearValue clearValues[2];
	clearValues[0].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	uint32_t width  = swapChain.GetWidth();
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

		VkPipeline pipeline = vulkanPipeline->GetVulkanPipeline();
		vkCmdBindPipeline(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// DRAW GEO HERE

		/**** BEGIN mesh geometry ****/
		{
			auto vulkanMeshVB = Hazel::Ref<Hazel::VulkanVertexBuffer>(mesh->GetVertexBuffer());
			VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCommandBuffer, 0, 1, &vbMeshBuffer, offsets);

			auto vulkanMeshIB = Hazel::Ref<Hazel::VulkanIndexBuffer>(mesh->GetIndexBuffer());
			VkBuffer ibMeshBuffer = vulkanMeshIB->GetVulkanBuffer();
			vkCmdBindIndexBuffer(drawCommandBuffer, ibMeshBuffer, 0, VK_INDEX_TYPE_UINT32);

			{
				// uniform buffer binding 0 uniform Camera
				void* ubPtr = shader->MapUniformBuffer(0);
				glm::mat4 proj = glm::perspectiveFov(glm::radians(45.0f), (float)swapChain.GetWidth(), (float)swapChain.GetHeight(), 0.1f, 1000.0f);
				// glm::mat4 view = glm::inverse(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 4.0f)));
				glm::mat4 viewProj = proj * viewMatrix; // Runtime camera
				// glm::mat4 viewProj = m_Camera.GetViewProjection(); // Editor camera
				memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
				shader->UnmapUniformBuffer(0);
			}

			uint32_t submeshIndex = 0;
			auto& submeshes = mesh->GetSubmeshes();
			for (Hazel::Submesh& submesh : submeshes)
			{
				// Descriptor Sets (Uniform buffers)
				// Bind descriptor sets describing shader binding points
				// VkDescriptorSet* descriptorSet = (VkDescriptorSet*)m_Mesh->GetDescriptorSet();
				VkPipelineLayout layout = vulkanPipeline->GetVulkanPipelineLayout();
				VkDescriptorSet descriptorSet = shader->GetDescriptorSet();
				vkCmdBindDescriptorSets(drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

				{
					// uniform buffer binding 1 uniform Transform
					void* ubPtr = shader->MapUniformBuffer(1);
					memcpy(ubPtr, &submesh.Transform, sizeof(glm::mat4));
					shader->UnmapUniformBuffer(1);
				}

				// Push Constants
				vkCmdPushConstants(drawCommandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &submesh.Transform);
				vkCmdPushConstants(drawCommandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), &m_RandomColors[submeshIndex++]);

				vkCmdDrawIndexed(drawCommandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
				// vkCmdDrawIndexed(drawCommandBuffer, vulkanMeshIB->GetCount(), 1, 0, 0, 0);
			}
		}

		vkCmdEndRenderPass(drawCommandBuffer);

		// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffer));
	}
}
