#pragma once

// The Vulkan version the renderer is made for on this target, in one place: the instance and the memory allocator ask for
// it, a device below it is refused (Renderer/VulkanSupport), the shaders are compiled to SPIR-V for it (Assets/ShaderCompile)
// and the window title names it. A newer driver (say Vulkan 1.4) still runs the renderer at this version; a target with
// another minimum (Android's 1.1, docs/android/Android_Export_Plan.html) sets its own here.

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>


constexpr uint32_t MoravaVulkanVersion = VK_API_VERSION_1_2;

// "Vulkan 1.2"
inline std::string MoravaVulkanVersionName()
{
	return "Vulkan " + std::to_string(VK_API_VERSION_MAJOR(MoravaVulkanVersion)) + "." + std::to_string(VK_API_VERSION_MINOR(MoravaVulkanVersion));
}
