/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#include "UniformBufferSetH2M.h"

#include "RendererAPI_H2M.h"
#include "H2M/Platform/Vulkan/VulkanUniformBufferSetH2M.h"


namespace H2M
{

	RefH2M<UniformBufferH2M> UniformBufferSetH2M::Create(uint32_t frames)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:     return RefH2M<UniformBufferH2M>();
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanUniformBufferSetH2M>::Create(frames);
		}

		H2M_CORE_ASSERT(false, "Unknown RendererAPI!");
		return RefH2M<UniformBufferH2M>();
	}

}
