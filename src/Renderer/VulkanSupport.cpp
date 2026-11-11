#include "VulkanSupport.h"
#include "VulkanTarget.h"

#include "Core/Log.h"

#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#endif


namespace VulkanSupport
{
	VkPhysicalDevice ChooseDevice(const std::vector<VkPhysicalDevice>& devices)
	{
		for (VkPhysicalDevice device : devices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				return device;
			}
		}
		return devices.empty() ? nullptr : devices.back();
	}

	static const char* const NoDriver = "This computer has no graphics card with a Vulkan driver, which this game needs to draw.";

	// What the renderer can't run without, on the device it would choose. Empty: all there
	static std::string FindMissing(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);
		const std::string name = properties.deviceName;
		if (properties.apiVersion < MoravaVulkanVersion)
		{
			return "The graphics card " + name + " has Vulkan " + std::to_string(VK_API_VERSION_MAJOR(properties.apiVersion)) + "." +
				std::to_string(VK_API_VERSION_MINOR(properties.apiVersion)) + ", and this game needs " + MoravaVulkanVersionName() + ".";
		}
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		if (!features.samplerAnisotropy || !features.fillModeNonSolid)
		{
			return "The graphics card " + name + " lacks a feature this game needs (" +
				(features.samplerAnisotropy ? "fillModeNonSolid" : "samplerAnisotropy") + ").";
		}
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
		for (const VkExtensionProperties& extension : extensions)
		{
			if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
			{
				return std::string();
			}
		}
		return "The graphics card " + name + " can't show Vulkan's pictures on the screen (no swapchain).";
	}

	bool Check(std::string& reason)
	{
#if defined(_WIN32)
		// The player delay-loads this library, so that it gets as far as here without it: Windows would stop it with a dialog
		// about a missing DLL. It stays loaded, for the calls that follow
		// (a damaged file would bring up Windows' own dialog, without this error mode)
		const UINT errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
		const HMODULE loader = LoadLibraryW(L"vulkan-1.dll");
		SetErrorMode(errorMode);
		if (!loader)
		{
			reason = "This computer doesn't have Vulkan, which this game needs to draw.";
			return false;
		}
#endif
		// MoltenVK is a "portability" implementation: the loader shows it only to an instance that asks for those
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		bool portability = false;
		for (const VkExtensionProperties& extension : extensions)
		{
			portability = portability || strcmp(extension.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0;
		}
		const char* portabilityExtension = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

		VkApplicationInfo application = {};
		application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application.apiVersion = MoravaVulkanVersion;
		VkInstanceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pApplicationInfo = &application;
		info.flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
		info.enabledExtensionCount = portability ? 1 : 0;
		info.ppEnabledExtensionNames = portability ? &portabilityExtension : nullptr;

		VkInstance instance = VK_NULL_HANDLE;
		const VkResult result = vkCreateInstance(&info, nullptr, &instance);
		if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
		{
			reason = NoDriver; // what the loader says when it has found no driver
			return false;
		}
		if (result != VK_SUCCESS)
		{
			Log::GetLogger()->error("Vulkan: vkCreateInstance returned {0}", (int)result);
			reason = "Vulkan could not be started on this computer.";
			return false;
		}

		uint32_t count = 0;
		vkEnumeratePhysicalDevices(instance, &count, nullptr);
		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(instance, &count, devices.data());
		devices.resize(count);
		if (devices.empty())
		{
			reason = NoDriver;
		}
		else
		{
			reason = FindMissing(ChooseDevice(devices));
		}
		vkDestroyInstance(instance, nullptr);
		return reason.empty();
	}
}
