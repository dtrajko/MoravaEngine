/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "UniformBufferH2M.h"

#include "H2M/Platform/OpenGL/OpenGLUniformBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanUniformBufferH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"


namespace H2M
{

	RefH2M<UniformBufferH2M> UniformBufferH2M::Create(uint32_t size, uint32_t binding)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   H2M_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return RefH2M<UniformBufferH2M>();
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLUniformBufferH2M>::Create(size, binding);
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanUniformBufferH2M>::Create(size, binding);
		}

		H2M_CORE_ASSERT(false, "Unknown RendererAPI!");
		return RefH2M<UniformBufferH2M>();
	}

}
