// No #pragma once: it undoes VulkanSyncShimH2M.h's macros wherever it is included (VulkanFrameSyncH2M.cpp calls the
// Vulkan functions themselves)
#undef vkDestroyBuffer
#undef vkDestroyBufferView
#undef vkDestroyImage
#undef vkDestroyImageView
#undef vkDestroySampler
#undef vkDestroyFramebuffer
#undef vkDestroyRenderPass
#undef vkDestroyPipeline
#undef vkDestroyPipelineLayout
#undef vkDestroyDescriptorPool
#undef vkDestroyDescriptorSetLayout
#undef vkDestroyQueryPool
#undef vkDestroyCommandPool
#undef vkFreeCommandBuffers
#undef vkFreeDescriptorSets
#undef vkResetDescriptorPool
#undef vkUpdateDescriptorSets
#undef vkFreeMemory
