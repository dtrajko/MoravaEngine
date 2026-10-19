#pragma once

#include "VulkanH2M.h"

#include <cstdint>


namespace H2M
{

	// Images shown in a user interface (ImGui::Image): the interface's backend registers an image and gives it an ID (ImGui's
	// Vulkan backend: a descriptor set, as an ImTextureID). The ImGui layer sets the backend while it is alive. Without one
	// (the player has no ImGui) no image is registered and every ID is 0, so the core's classes don't call ImGui.
	class VulkanUIImagesH2M
	{
	public:
		using AddFunction = uint64_t(*)(VkSampler sampler, VkImageView view, VkImageLayout layout);
		using RemoveFunction = void(*)(uint64_t id);

		// nullptr, nullptr: none (before the backend shuts down: it frees its images itself)
		static void SetBackend(AddFunction add, RemoveFunction remove);
		static bool HasBackend();

		static uint64_t Add(VkSampler sampler, VkImageView view, VkImageLayout layout); // 0 without a backend
		static void Remove(uint64_t id);
	};

}
