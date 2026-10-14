/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#include "UniformBufferSetH2M.h"

#include "RendererAPI_H2M.h"
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanUniformBufferSetH2M.h"
#endif


namespace H2M
{

	RefH2M<UniformBufferSetH2M> UniformBufferSetH2M::Create(uint32_t frames)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<UniformBufferSetH2M>();
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanUniformBufferSetH2M>::Create(frames);
#endif
		}

		H2M_CORE_ASSERT(false, "Unknown RendererAPI!");
		return RefH2M<UniformBufferSetH2M>();
	}

}
