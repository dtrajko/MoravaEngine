#include "EnvMapVulkanTextureViews.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Camera/Camera.h"
#include "Core/Log.h"
#include "EnvMapVulkanProfiler.h"
#include "EnvMapVulkanScene.h"
#include "EnvMapVulkanSceneData.h"
#include "EnvMapVulkanWater.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>


static constexpr uint32_t FrameSet = 0; // the PBR shaders' per-frame set

// A host visible uniform buffer of the given size (as the water's reflection makes its own)
static void CreateUniformBuffer(H2M::VulkanShaderH2M::UniformBufferH2M& buffer, uint32_t size)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.Buffer));
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, buffer.Buffer, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("TextureView"));
	allocator.Allocate(requirements, &buffer.Memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, buffer.Buffer, buffer.Memory, 0));
	buffer.Size = size;
	buffer.Descriptor = { buffer.Buffer, 0, size };
}

static void WriteUniformBuffer(const H2M::VulkanShaderH2M::UniformBufferH2M& buffer, const void* data, uint32_t size)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	void* mapped;
	VK_CHECK_RESULT_H2M(vkMapMemory(device, buffer.Memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	vkUnmapMemory(device, buffer.Memory);
}

static void DestroyUniformBuffer(H2M::VulkanShaderH2M::UniformBufferH2M& buffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (buffer.Buffer) vkDestroyBuffer(device, buffer.Buffer, nullptr);
	if (buffer.Memory) vkFreeMemory(device, buffer.Memory, nullptr);
	buffer.Buffer = VK_NULL_HANDLE;
	buffer.Memory = VK_NULL_HANDLE;
}

static VkImageMemoryBarrier ImageBarrier(VkImage image, uint32_t baseMip, uint32_t mipCount, VkImageLayout oldLayout, VkImageLayout newLayout,
	VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, baseMip, mipCount, 0, 1 };
	return barrier;
}

static H2M::RefH2M<H2M::VulkanShaderH2M> GetShader(const char* name)
{
	return H2M::RendererH2M::GetShaderLibrary()->Get(name).As<H2M::VulkanShaderH2M>();
}

static H2M::RefH2M<H2M::FramebufferH2M> CreateFramebuffer(H2M::ImageFormatH2M colorFormat, uint32_t width, uint32_t height, const char* name)
{
	H2M::FramebufferSpecificationH2M spec;
	// The scene's: as the main view's framebuffer (a compatible render pass for the mesh and skybox pipelines)
	spec.Attachments = { colorFormat, H2M::ImageFormatH2M::Depth };
	spec.Width = width;
	spec.Height = height;
	spec.CopySource = true; // copied into the render texture
	spec.DebugName = name;
	return H2M::FramebufferH2M::Create(spec);
}

void EnvMapVulkanTextureViews::Create(const H2M::RefH2M<H2M::VertexBufferH2M>& quadVertexBuffer, const H2M::RefH2M<H2M::IndexBufferH2M>& quadIndexBuffer)
{
	m_QuadVertexBuffer = quadVertexBuffer;
	m_QuadIndexBuffer = quadIndexBuffer;

	// The composite's pipeline, for every view's composite framebuffer (all of the same formats)
	H2M::PipelineSpecificationH2M pipelineSpecification;
	pipelineSpecification.Layout = {
		{ H2M::ShaderDataTypeH2M::Float3, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
	};
	pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("RenderTextureComposite");
	H2M::RenderPassSpecificationH2M renderPassSpec;
	renderPassSpec.TargetFramebuffer = CreateFramebuffer(H2M::ImageFormatH2M::RGBA, 16, 16, "RenderTextureComposite");
	pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
	pipelineSpecification.DebugName = "RenderTextureComposite";
	m_CompositePipeline = H2M::PipelineH2M::Create(pipelineSpecification);
}

void EnvMapVulkanTextureViews::Destroy()
{
	for (auto& [entity, view] : m_Views)
	{
		DestroyView(view);
	}
	m_Views.clear();
	m_CompositePipeline = {};
	m_QuadVertexBuffer = {};
	m_QuadIndexBuffer = {};
}

void EnvMapVulkanTextureViews::CreateView(View& view, const FrameInputs& inputs)
{
	// Set 0: the PBR shaders' per-frame layout. Camera and SceneData are the view's own; no ambient occlusion and no
	// ray-traced reflections (those images belong to the main view's pixels); the rest is copied from the main set (Update)
	H2M::RefH2M<H2M::VulkanShaderH2M> pbrShader = GetShader("HazelPBR_Static");
	view.FrameSet = pbrShader->CreateDescriptorSets(FrameSet);
	CreateUniformBuffer(view.CameraBuffer, 2 * sizeof(glm::mat4)); // the Camera block: view-projection, previous view-projection
	CreateUniformBuffer(view.SceneDataBuffer, sizeof(EnvMapVulkanSceneDataGPU));
	std::array<VkWriteDescriptorSet, 4> writes;
	const char* names[4] = { "Camera", "SceneData", "u_AOImage", "u_ReflectionImage" };
	for (size_t i = 0; i < writes.size(); i++)
	{
		writes[i] = *pbrShader->GetDescriptorSet(names[i], FrameSet);
		writes[i].dstSet = view.FrameSet.DescriptorSets[0];
		writes[i].descriptorCount = 1;
	}
	writes[0].pBufferInfo = &view.CameraBuffer.Descriptor;
	writes[1].pBufferInfo = &view.SceneDataBuffer.Descriptor;
	writes[2].pImageInfo = &inputs.WhiteAmbientOcclusion;
	writes[3].pImageInfo = &inputs.NoReflections;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

	// The skybox's set: its own camera (the skybox shader's buffer is the main view's); the cube map is written in Update
	H2M::RefH2M<H2M::VulkanShaderH2M> skyboxShader = GetShader("Skybox");
	view.SkyboxSet = skyboxShader->CreateDescriptorSets();
	CreateUniformBuffer(view.SkyboxCameraBuffer, 2 * sizeof(glm::mat4)); // view-projection, its inverse
	VkWriteDescriptorSet skyboxWrite = *skyboxShader->GetDescriptorSet("Camera");
	skyboxWrite.dstSet = view.SkyboxSet.DescriptorSets[0];
	skyboxWrite.descriptorCount = 1;
	skyboxWrite.pBufferInfo = &view.SkyboxCameraBuffer.Descriptor;
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &skyboxWrite, 0, nullptr);
	view.SkyboxEnvironmentVersion = 0;

	view.CompositeSet = m_CompositePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>()->CreateDescriptorSets();

	view.Draw.FrameSet = view.FrameSet.DescriptorSets[0];
	view.Draw.SkyboxSet = view.SkyboxSet.DescriptorSets[0];
}

void EnvMapVulkanTextureViews::ResizeView(View& view, uint32_t width, uint32_t height)
{
	if (view.SceneFramebuffer)
	{
		view.SceneFramebuffer->Resize(width, height); // waits for the device
		view.CompositeFramebuffer->Resize(width, height);
	}
	else
	{
		view.SceneFramebuffer = CreateFramebuffer(H2M::ImageFormatH2M::RGBA16F, width, height, "TextureView");
		view.CompositeFramebuffer = CreateFramebuffer(H2M::ImageFormatH2M::RGBA, width, height, "TextureViewComposite");
		view.Glass = std::make_unique<EnvMapVulkanGlass>();
		view.Glass->Create(view.SceneFramebuffer);
	}
	view.Glass->Resize(width, height);
	view.Width = width;
	view.Height = height;
	view.Draw.SceneFramebuffer = view.SceneFramebuffer;
	view.Draw.Glass = view.Glass.get();

	// The composite reads the scene framebuffer's new image
	H2M::RefH2M<H2M::VulkanShaderH2M> compositeShader = m_CompositePipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
	VkWriteDescriptorSet write = *compositeShader->GetDescriptorSet("u_Texture");
	write.dstSet = view.CompositeSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &view.SceneFramebuffer.As<H2M::VulkanFramebufferH2M>()->GetVulkanDescriptorInfo();
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
}

void EnvMapVulkanTextureViews::DestroyView(View& view)
{
	DestroyUniformBuffer(view.CameraBuffer);
	DestroyUniformBuffer(view.SceneDataBuffer);
	DestroyUniformBuffer(view.SkyboxCameraBuffer);
	view.FrameSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	view.SkyboxSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	view.CompositeSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	if (view.Glass)
	{
		view.Glass->Destroy();
		view.Glass.reset();
	}
	view.Draw = ViewDraw();
	view.SceneFramebuffer = {};
	view.CompositeFramebuffer = {};
	view.RenderTexture = nullptr;
}

void EnvMapVulkanTextureViews::Update(const EnvMapVulkanScene& scene, const FrameInputs& inputs)
{
	m_Exposure = inputs.Exposure;
	m_HuePreservation = inputs.HuePreservation;
	m_ActiveViews = 0;
	for (auto& [entity, view] : m_Views)
	{
		view.Active = false;
	}

	scene.Each<CameraComponent>([&](EnvMapVulkanEntityID entity, const CameraComponent& lens)
	{
		if (lens.Target.empty())
		{
			return;
		}
		auto [it, created] = m_Views.try_emplace(entity);
		View& view = it->second;
		if (created)
		{
			CreateView(view, inputs);
			view.Stagger = m_ViewsCreated++;
		}
		if (created || view.TargetFile != lens.Target)
		{
			// Loaded once per file: a file that can't be read is reported once, not every frame
			view.TargetFile = lens.Target;
			view.RenderTexture = EnvMapVulkanRenderTexture::Get(lens.Target);
		}
		view.Enabled = lens.Enabled;
		if (!view.RenderTexture || !view.RenderTexture->GetTexture())
		{
			return;
		}
		if (!lens.Enabled)
		{
			view.FramesUntilDraw = 0; // its texture keeps the last picture; enabled again, it draws at once
			return;
		}
		const RenderTextureSettings& settings = view.RenderTexture->GetSettings();
		if (settings.Width != view.Width || settings.Height != view.Height)
		{
			ResizeView(view, settings.Width, settings.Height);
			view.FramesUntilDraw = 0; // a new texture: its picture at once
		}

		// Every DrawEvery frames (the texture keeps its picture in between). A new view draws at once; after that the views
		// made together are spread over the frames, so they don't all draw in the same one
		const uint32_t drawEvery = std::max(lens.DrawEvery, 1u);
		view.FramesUntilDraw = std::min(view.FramesUntilDraw, 2 * (drawEvery - 1)); // Draw Every made smaller: no long wait
		if (view.FramesUntilDraw > 0)
		{
			view.FramesUntilDraw--;
			return;
		}
		view.FramesUntilDraw = (drawEvery - 1) + (view.Drawn ? 0 : view.Stagger % drawEvery);
		view.Drawn = true;

		// The camera as the game camera is placed (PlaceGameCamera), with the texture's aspect ratio
		Camera camera;
		EnvMapVulkanSceneData::PlaceGameCamera(camera, scene, entity);
		camera.SetViewportSize((float)view.Width, (float)view.Height);
		view.Draw.ViewProjection = camera.GetViewProjection();
		view.Draw.Position = camera.GetPosition();

		// The view writes no motion: its previous view-projection is this frame's
		const glm::mat4 cameraMatrices[2] = { view.Draw.ViewProjection, view.Draw.ViewProjection };
		WriteUniformBuffer(view.CameraBuffer, cameraMatrices, sizeof(cameraMatrices));
		const glm::mat4 skyboxMatrices[2] = { view.Draw.ViewProjection, glm::inverse(view.Draw.ViewProjection) };
		WriteUniformBuffer(view.SkyboxCameraBuffer, skyboxMatrices, sizeof(skyboxMatrices));
		EnvMapVulkanSceneDataGPU sceneData;
		sceneData.CameraPosition = view.Draw.Position;
		sceneData.EnvMapRotation = inputs.EnvMapRotation;
		sceneData.DisplayTone = glm::vec4(inputs.Exposure, inputs.HuePreservation, 0.0f, 0.0f); // its composite's (a Color texture)
		if (inputs.Water)
		{
			FillWaterSceneData(*inputs.Water, view.Draw.Position, inputs.SunDirection, sceneData);
		}
		WriteUniformBuffer(view.SceneDataBuffer, &sceneData, sizeof(sceneData));

		// The rest of set 0 as in the main per-frame set (environment maps, BRDF LUT, lights, shadows, caustics: bindings 2
		// to 10, the probe volume: 11 to 14, and the material buffer: 17), as the water's reflection does
		const uint32_t copiedBindings[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17 };
		std::array<VkCopyDescriptorSet, 14> copies;
		for (uint32_t i = 0; i < (uint32_t)copies.size(); i++)
		{
			VkCopyDescriptorSet& copy = copies[i];
			copy = {};
			copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
			copy.srcSet = inputs.MainFrameSet;
			copy.srcBinding = copiedBindings[i];
			copy.dstSet = view.FrameSet.DescriptorSets[0];
			copy.dstBinding = copiedBindings[i];
			copy.descriptorCount = 1;
		}
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 0, nullptr, (uint32_t)copies.size(), copies.data());

		// The skybox's cube map, when the environment has new cube maps
		if (view.SkyboxEnvironmentVersion != inputs.EnvironmentVersion)
		{
			VkWriteDescriptorSet write = *GetShader("Skybox")->GetDescriptorSet("u_Texture");
			write.dstSet = view.SkyboxSet.DescriptorSets[0];
			write.descriptorCount = 1;
			write.pImageInfo = &inputs.SkyboxCube;
			vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
			view.SkyboxEnvironmentVersion = inputs.EnvironmentVersion;
		}

		view.Active = true;
		m_ActiveViews++;
	});

	// The views of cameras that are gone (deleted, or without a target now)
	for (auto it = m_Views.begin(); it != m_Views.end();)
	{
		const CameraComponent* lens = scene.TryGet<CameraComponent>(it->first);
		if (!lens || lens->Target.empty())
		{
			vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice());
			DestroyView(it->second);
			it = m_Views.erase(it);
		}
		else
		{
			++it;
		}
	}
}

std::vector<EnvMapVulkanEntityID> EnvMapVulkanTextureViews::GetCameras() const
{
	std::vector<EnvMapVulkanEntityID> cameras;
	for (const auto& [entity, view] : m_Views)
	{
		if (view.RenderTexture && view.Drawn && view.Enabled)
		{
			cameras.push_back(entity);
		}
	}
	return cameras;
}

std::string EnvMapVulkanTextureViews::GetCameraLabel(EnvMapVulkanEntityID camera)
{
	const std::string id = std::to_string(camera);
	return "Camera #" + (id.size() > 6 ? id.substr(id.size() - 6) : id);
}

void EnvMapVulkanTextureViews::Record(VkCommandBuffer commandBuffer, const std::function<void(VkCommandBuffer, const ViewDraw&)>& drawScene)
{
	for (auto& [entity, view] : m_Views)
	{
		if (!view.Active)
		{
			continue;
		}

		// Each camera's own scope in the profiler (inside the renderer's Render Textures): its time in the debug overlay
		EnvMapVulkanProfiler::Scope cameraScope(commandBuffer, GetCameraLabel(entity), "One camera that draws into a render texture "
			"(the last digits of its entity ID): its view, composite and copy into the texture.");

		// The scene: cleared to black (the skybox covers what no mesh does)
		H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = view.SceneFramebuffer.As<H2M::VulkanFramebufferH2M>();
		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = framebuffer->GetRenderPass();
		beginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();
		beginInfo.renderArea.extent = { view.Width, view.Height };
		beginInfo.clearValueCount = 2;
		beginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		EnvMapVulkanProfiler::CountRenderPass(view.Width, view.Height);
		const VkViewport viewport = { 0.0f, 0.0f, (float)view.Width, (float)view.Height, 0.0f, 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		const VkRect2D scissor = { { 0, 0 }, { view.Width, view.Height } };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		view.Draw.BeginInfo = &beginInfo;
		view.Draw.Viewport = viewport;
		view.Draw.Scissor = scissor;
		drawScene(commandBuffer, view.Draw);
		view.Draw.BeginInfo = nullptr;
		vkCmdEndRenderPass(commandBuffer); // leaves the color image in SHADER_READ_ONLY (the continue render pass too)

		// A Color texture: exposed and tonemapped into the composite framebuffer
		if (view.RenderTexture->GetSettings().Format == RenderTextureFormat::Color)
		{
			H2M::RefH2M<H2M::VulkanFramebufferH2M> composite = view.CompositeFramebuffer.As<H2M::VulkanFramebufferH2M>();
			H2M::RefH2M<H2M::VulkanPipelineH2M> pipeline = m_CompositePipeline.As<H2M::VulkanPipelineH2M>();
			VkRenderPassBeginInfo compositeBeginInfo = beginInfo;
			compositeBeginInfo.renderPass = composite->GetRenderPass();
			compositeBeginInfo.framebuffer = composite->GetVulkanFramebuffer();
			vkCmdBeginRenderPass(commandBuffer, &compositeBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			EnvMapVulkanProfiler::CountRenderPass(view.Width, view.Height);
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			VkBuffer vertexBuffer = m_QuadVertexBuffer.As<H2M::VulkanVertexBufferH2M>()->GetVulkanBuffer();
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
			vkCmdBindIndexBuffer(commandBuffer, m_QuadIndexBuffer.As<H2M::VulkanIndexBufferH2M>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanPipeline());
			VkPipelineLayout layout = pipeline->GetVulkanPipelineLayout();
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, view.CompositeSet.DescriptorSets.data(), 0, nullptr);
			const float uniforms[2] = { m_Exposure, m_HuePreservation };
			vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uniforms), uniforms);
			vkCmdDrawIndexed(commandBuffer, m_QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
			EnvMapVulkanProfiler::CountDraw(m_QuadIndexBuffer->GetCount() / 3);
			vkCmdEndRenderPass(commandBuffer);
		}

		RecordCopy(commandBuffer, view);
	}
}

// The view's picture into the render texture's first mip level, then the smaller levels made from it again (as
// VulkanTexture2D_H2M::GenerateMips does): what showed the texture before reads it again from the fragment and compute
// shaders (the materials, ray tracing, the UI)
void EnvMapVulkanTextureViews::RecordCopy(VkCommandBuffer commandBuffer, View& view)
{
	H2M::RefH2M<H2M::VulkanTexture2D_H2M> texture = view.RenderTexture->GetTexture().As<H2M::VulkanTexture2D_H2M>();
	const VkImage image = texture->GetVulkanImage();
	const uint32_t mipCount = std::max(texture->GetMipLevelCount(), 1u);
	const bool color = view.RenderTexture->GetSettings().Format == RenderTextureFormat::Color;
	const VkImage source = (color ? view.CompositeFramebuffer : view.SceneFramebuffer).As<H2M::VulkanFramebufferH2M>()->GetColorVulkanImage();

	// The source from its render pass (SHADER_READ_ONLY), the texture from what read it (every level SHADER_READ_ONLY)
	const VkImageMemoryBarrier toTransfer[2] = {
		ImageBarrier(source, 0, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT),
		ImageBarrier(image, 0, mipCount, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT) };
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, toTransfer);

	if (color)
	{
		// Gamma encoded 8-bit values into the sRGB texture as they are: sampled, they are decoded to linear light
		VkImageCopy region = {};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.extent = { view.Width, view.Height, 1 };
		vkCmdCopyImage(commandBuffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}
	else
	{
		// Half floats into the texture's floats: a blit converts them (the same size: nothing is filtered)
		VkImageBlit region = {};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.srcOffsets[1] = { (int32_t)view.Width, (int32_t)view.Height, 1 };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		region.dstOffsets[1] = { (int32_t)view.Width, (int32_t)view.Height, 1 };
		vkCmdBlitImage(commandBuffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);
	}

	// Each smaller level from the one before it
	for (uint32_t level = 1; level < mipCount; level++)
	{
		const VkImageMemoryBarrier previous = ImageBarrier(image, level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &previous);
		VkImageBlit region = {};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1 };
		region.srcOffsets[1] = { (int32_t)std::max(view.Width >> (level - 1), 1u), (int32_t)std::max(view.Height >> (level - 1), 1u), 1 };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
		region.dstOffsets[1] = { (int32_t)std::max(view.Width >> level, 1u), (int32_t)std::max(view.Height >> level, 1u), 1 };
		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
	}

	// Every level back to SHADER_READ_ONLY: the last from TRANSFER_DST, the others from TRANSFER_SRC
	std::vector<VkImageMemoryBarrier> toRead;
	if (mipCount > 1)
	{
		toRead.push_back(ImageBarrier(image, 0, mipCount - 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT));
	}
	toRead.push_back(ImageBarrier(image, mipCount - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT));
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, (uint32_t)toRead.size(), toRead.data());
}
