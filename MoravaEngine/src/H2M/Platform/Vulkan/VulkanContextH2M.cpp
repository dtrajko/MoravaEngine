/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanContextH2M.h"

#include "VulkanH2M.h"

#include <glfw/glfw3.h>


namespace H2M
{

	static bool s_Validation = true;

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
	{
		(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
		MORAVA_CORE_WARN("VulkanDebugCallback:\n  Object Type: {0}\n  Message: {1}", objectType, pMessage);
		return VK_FALSE;
	}

	VulkanContextH2M::VulkanContextH2M(Window* window)
		: m_Window(window)
	{
	}

	VulkanContextH2M::~VulkanContextH2M()
	{
		m_Device->Destroy();

		vkDestroyInstance(s_VulkanInstance, nullptr);
		s_VulkanInstance = nullptr;
	}

	void VulkanContextH2M::Init()
	{
		MORAVA_CORE_INFO("VulkanContextH2M::Init");

		H2M_CORE_ASSERT(glfwVulkanSupported(), "GLFW must support Vulkan!");

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Application Info
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hazel";
		appInfo.pEngineName = "Hazel";
		appInfo.apiVersion = VK_API_VERSION_1_2;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Extensions and Validation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		if (s_Validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		VkValidationFeaturesEXT features = {};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.enabledValidationFeatureCount = 1;
		features.pEnabledValidationFeatures = enables;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;// &features;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		// TODO: Extract all validation into separate class
		if (s_Validation)
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation"; // VK_LAYER_LUNARG_standard_validation
			// Check if this layer is available at instance level
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			bool validationLayerPresent = false;
			MORAVA_CORE_TRACE("Vulkan Instance Layers:");
			for (const VkLayerProperties& layer : instanceLayerProperties)
			{
				MORAVA_CORE_TRACE("  {0}", layer.layerName);
				if (strcmp(layer.layerName, validationLayerName) == 0)
				{
					validationLayerPresent = true;
					break;
				}
			}
			if (validationLayerPresent)
			{
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
				MORAVA_CORE_ERROR("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Instance and Surface Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VK_CHECK_RESULT_H2M(vkCreateInstance(&instanceCreateInfo, nullptr, &s_VulkanInstance));

		if (s_Validation)
		{
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(s_VulkanInstance, "vkCreateDebugReportCallbackEXT");
			H2M_CORE_ASSERT(vkCreateDebugReportCallbackEXT != NULL, "");
			VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
			debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debug_report_ci.pfnCallback = VulkanDebugReportCallback;
			debug_report_ci.pUserData = NULL;
			VK_CHECK_RESULT_H2M(vkCreateDebugReportCallbackEXT(s_VulkanInstance, &debug_report_ci, nullptr, &m_DebugReportCallback));
		}

		m_PhysicalDevice = VulkanPhysicalDeviceH2M::Select();

		VkPhysicalDeviceFeatures enabledFeatures;
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		enabledFeatures.wideLines = true;
		enabledFeatures.fillModeNonSolid = true;
		enabledFeatures.pipelineStatisticsQuery = true;
		m_Device = RefH2M<VulkanDeviceH2M>::Create(m_PhysicalDevice, enabledFeatures);

		// Why is this here?
		m_Allocator = VulkanAllocatorH2M(m_Device, std::string("Default"));

		// InitPartTwoTemp();

		// Pipeline Cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT_H2M(vkCreatePipelineCache(m_Device->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}

	void VulkanContextH2M::OnResize(uint32_t width, uint32_t height)
	{
		m_Window->GetSwapChain().OnResize(width, height);
	}

	void VulkanContextH2M::BeginFrame()
	{
		m_Window->GetSwapChain().BeginFrame();
	}

	void VulkanContextH2M::SwapBuffers()
	{
		m_Window->GetSwapChain().Present();
	}

}
