#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>


/**
 * Can this machine run the engine's Vulkan renderer? Asked before a window exists, so a machine that can't is told so in a
 * sentence instead of a crash (docs/engine/Game_Engine_Runtime_Guide.html, 11.4).
 */
namespace VulkanSupport
{
	// The GPU the engine uses: the first discrete one, else the last. nullptr: none
	VkPhysicalDevice ChooseDevice(const std::vector<VkPhysicalDevice>& devices);

	// True: this machine has what the renderer requires. False: reason says what is missing, in words for a player
	bool Check(std::string& reason);
}
