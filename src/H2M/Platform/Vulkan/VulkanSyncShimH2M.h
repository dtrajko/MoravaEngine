#pragma once

// Frames in flight (VulkanFrameSyncH2M.h): the Vulkan functions that change or destroy what a frame on the GPU may still
// use wait for that frame first (free when it is done already). In every source file of the engine through the
// precompiled header, so no call is forgotten, also in code written later.
//
// Function-like macros: only calls are changed ("vkDestroyBuffer(device, buffer, nullptr)"), not a function's address
// or a struct member of the same name (VMA's table of Vulkan functions). Inside its own expansion a macro's name is the
// function itself, so the real function is called after the wait.

#include <vulkan/vulkan.h>

namespace H2M::VulkanFrameSync
{
	void WaitForGpu();
	void ForgetMemory(VkDeviceMemory memory);
	void CountDescriptorUpdate();
}

#define MORAVA_VK_WAIT_THEN(call) (::H2M::VulkanFrameSync::WaitForGpu(), call)

#define vkDestroyBuffer(...) MORAVA_VK_WAIT_THEN(vkDestroyBuffer(__VA_ARGS__))
#define vkDestroyBufferView(...) MORAVA_VK_WAIT_THEN(vkDestroyBufferView(__VA_ARGS__))
#define vkDestroyImage(...) MORAVA_VK_WAIT_THEN(vkDestroyImage(__VA_ARGS__))
#define vkDestroyImageView(...) MORAVA_VK_WAIT_THEN(vkDestroyImageView(__VA_ARGS__))
#define vkDestroySampler(...) MORAVA_VK_WAIT_THEN(vkDestroySampler(__VA_ARGS__))
#define vkDestroyFramebuffer(...) MORAVA_VK_WAIT_THEN(vkDestroyFramebuffer(__VA_ARGS__))
#define vkDestroyRenderPass(...) MORAVA_VK_WAIT_THEN(vkDestroyRenderPass(__VA_ARGS__))
#define vkDestroyPipeline(...) MORAVA_VK_WAIT_THEN(vkDestroyPipeline(__VA_ARGS__))
#define vkDestroyPipelineLayout(...) MORAVA_VK_WAIT_THEN(vkDestroyPipelineLayout(__VA_ARGS__))
#define vkDestroyDescriptorPool(...) MORAVA_VK_WAIT_THEN(vkDestroyDescriptorPool(__VA_ARGS__))
#define vkDestroyDescriptorSetLayout(...) MORAVA_VK_WAIT_THEN(vkDestroyDescriptorSetLayout(__VA_ARGS__))
#define vkDestroyQueryPool(...) MORAVA_VK_WAIT_THEN(vkDestroyQueryPool(__VA_ARGS__))
#define vkDestroyCommandPool(...) MORAVA_VK_WAIT_THEN(vkDestroyCommandPool(__VA_ARGS__))
#define vkFreeCommandBuffers(...) MORAVA_VK_WAIT_THEN(vkFreeCommandBuffers(__VA_ARGS__))
#define vkFreeDescriptorSets(...) MORAVA_VK_WAIT_THEN(vkFreeDescriptorSets(__VA_ARGS__))
#define vkResetDescriptorPool(...) MORAVA_VK_WAIT_THEN(vkResetDescriptorPool(__VA_ARGS__))
// Counted: a copy of a descriptor set can be skipped while nothing was written since (GetDescriptorUpdateCount)
#define vkUpdateDescriptorSets(...) (::H2M::VulkanFrameSync::WaitForGpu(), ::H2M::VulkanFrameSync::CountDescriptorUpdate(), vkUpdateDescriptorSets(__VA_ARGS__))
// Freed memory: also its kept writes go (a new allocation may get the same handle)
#define vkFreeMemory(device, memory, allocator) \
	(::H2M::VulkanFrameSync::WaitForGpu(), ::H2M::VulkanFrameSync::ForgetMemory(memory), vkFreeMemory(device, memory, allocator))
