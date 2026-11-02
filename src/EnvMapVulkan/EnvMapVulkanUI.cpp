#include "EnvMapVulkanUI.h"

#include "EnvMapVulkanProfiler.h"
#include "UI/UILayout.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <cstring>


static_assert(sizeof(UIVertex) == 40, "UIVertex must match the vertex inputs of Resources/Shaders/UI.glsl");

// A texture's descriptor set and a replaced buffer are freed this many frames after their last use: the GPU is done with
// them by then (the swapchain waits for the previous frame)
static constexpr uint64_t RetireFrames = 3;

void EnvMapVulkanUI::Create(H2M::RefH2M<H2M::FramebufferH2M> target)
{
	H2M::PipelineSpecificationH2M pipelineSpecification;
	pipelineSpecification.Layout = {
		{ H2M::ShaderDataTypeH2M::Float2, "a_Position" },
		{ H2M::ShaderDataTypeH2M::Float2, "a_TexCoord" },
		{ H2M::ShaderDataTypeH2M::Float4, "a_Color" },
		{ H2M::ShaderDataTypeH2M::Float, "a_Mode" },
		{ H2M::ShaderDataTypeH2M::Float, "a_DistancePixels" },
	};
	pipelineSpecification.Shader = H2M::RendererH2M::GetShaderLibrary()->Get("UI");
	pipelineSpecification.BackfaceCulling = false;
	pipelineSpecification.DepthTest = false;
	pipelineSpecification.DepthWrite = false;
	pipelineSpecification.WriteAlpha = false;
	H2M::RenderPassSpecificationH2M renderPassSpec;
	renderPassSpec.TargetFramebuffer = target;
	pipelineSpecification.RenderPass = H2M::RenderPassH2M::Create(renderPassSpec);
	pipelineSpecification.DebugName = "UI";
	m_Pipeline = H2M::PipelineH2M::Create(pipelineSpecification);

	const uint32_t white = 0xFFFFFFFF;
	m_White = H2M::Texture2D_H2M::Create(H2M::ImageFormatH2M::RGBA, 1, 1, &white);
}

void EnvMapVulkanUI::Destroy()
{
	FreeRetired(true);
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (auto& [texture, set] : m_TextureSets)
	{
		vkDestroyDescriptorPool(device, set.Pool, nullptr);
	}
	m_TextureSets.clear();
	FreeBuffer(m_Vertices);
	FreeBuffer(m_Indices);
	m_White = {};
	m_Pipeline = {};
}

void EnvMapVulkanUI::FreeBuffer(Buffer& buffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (buffer.Mapped) vkUnmapMemory(device, buffer.Memory);
	if (buffer.Handle) vkDestroyBuffer(device, buffer.Handle, nullptr);
	if (buffer.Memory) vkFreeMemory(device, buffer.Memory, nullptr);
	buffer = Buffer();
}

void EnvMapVulkanUI::FreeRetired(bool all)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (auto it = m_Retired.begin(); it != m_Retired.end();)
	{
		if (all || m_Frame >= it->Frame + RetireFrames)
		{
			FreeBuffer(it->OldBuffer);
			if (it->Pool) vkDestroyDescriptorPool(device, it->Pool, nullptr);
			it = m_Retired.erase(it);
		}
		else
		{
			++it;
		}
	}
}

// Room for size bytes: a new, larger buffer when it has less (twice the size, so a growing UI doesn't make one per frame)
void EnvMapVulkanUI::Reserve(Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage)
{
	if (buffer.Capacity >= size)
	{
		return;
	}
	if (buffer.Handle)
	{
		m_Retired.push_back({ buffer, VK_NULL_HANDLE, m_Frame });
		buffer = Buffer();
	}
	VkDeviceSize capacity = 16 * 1024;
	while (capacity < size)
	{
		capacity *= 2;
	}
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = capacity;
	bufferInfo.usage = usage;
	VK_CHECK_RESULT_H2M(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.Handle));
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, buffer.Handle, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("UIBuffer"));
	allocator.Allocate(requirements, &buffer.Memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT_H2M(vkBindBufferMemory(device, buffer.Handle, buffer.Memory, 0));
	VK_CHECK_RESULT_H2M(vkMapMemory(device, buffer.Memory, 0, capacity, 0, &buffer.Mapped));
	buffer.Capacity = capacity;
}

VkDescriptorSet EnvMapVulkanUI::GetTextureSet(const H2M::RefH2M<H2M::Texture2D_H2M>& texture)
{
	const H2M::RefH2M<H2M::Texture2D_H2M>& used = texture ? texture : m_White;
	auto it = m_TextureSets.find(used.Raw());
	if (it == m_TextureSets.end())
	{
		H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_Pipeline->GetSpecification().Shader.As<H2M::VulkanShaderH2M>();
		H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet created = shader->CreateDescriptorSets();
		TextureSet set;
		set.Texture = used;
		set.Pool = created.Pool;
		set.Set = created.DescriptorSets[0];
		VkWriteDescriptorSet write = *shader->GetDescriptorSet("u_Texture");
		write.dstSet = set.Set;
		write.descriptorCount = 1;
		write.pImageInfo = &used.As<H2M::VulkanTexture2D_H2M>()->GetVulkanDescriptorInfo();
		vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), 1, &write, 0, nullptr);
		it = m_TextureSets.emplace(used.Raw(), set).first;
	}
	it->second.LastFrame = m_Frame;
	return it->second.Set;
}

void EnvMapVulkanUI::Record(VkCommandBuffer commandBuffer, const UIDrawList& list, uint32_t width, uint32_t height, float exposure,
	float huePreservation)
{
	m_Frame++;
	FreeRetired(false);
	// Textures no longer drawn: their sets go (and the references that kept them)
	for (auto it = m_TextureSets.begin(); it != m_TextureSets.end();)
	{
		if (it->second.Texture != m_White && m_Frame > it->second.LastFrame + RetireFrames)
		{
			m_Retired.push_back({ Buffer(), it->second.Pool, m_Frame });
			it = m_TextureSets.erase(it);
		}
		else
		{
			++it;
		}
	}
	if (list.IsEmpty() || !IsValid())
	{
		return;
	}

	const VkDeviceSize vertexBytes = list.Vertices.size() * sizeof(UIVertex);
	const VkDeviceSize indexBytes = list.Indices.size() * sizeof(uint32_t);
	Reserve(m_Vertices, vertexBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	Reserve(m_Indices, indexBytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	memcpy(m_Vertices.Mapped, list.Vertices.data(), vertexBytes);
	memcpy(m_Indices.Mapped, list.Indices.data(), indexBytes);

	H2M::RefH2M<H2M::VulkanPipelineH2M> pipeline = m_Pipeline.As<H2M::VulkanPipelineH2M>();
	VkPipelineLayout layout = pipeline->GetVulkanPipelineLayout();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetVulkanPipeline());
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Vertices.Handle, &offset);
	vkCmdBindIndexBuffer(commandBuffer, m_Indices.Handle, 0, VK_INDEX_TYPE_UINT32);
	const glm::vec2 viewSize((float)width, (float)height);
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec2), &viewSize);
	const float tone[2] = { exposure, huePreservation }; // UI.glsl's Tone block, after the view's size
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 16, sizeof(tone), tone);

	for (const UIDrawBatch& batch : list.Batches)
	{
		VkDescriptorSet set = GetTextureSet(batch.Texture);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, batch.IndexCount, 1, batch.FirstIndex, 0, 0);
		EnvMapVulkanProfiler::CountDraw(batch.IndexCount / 3);
	}
}
