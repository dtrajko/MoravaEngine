/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "IndexBufferH2M.h"

#include "H2M/Platform/OpenGL/OpenGLIndexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanIndexBufferH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/DX11/DX11IndexBuffer.h"


namespace H2M
{

	RefH2M<IndexBufferH2M> IndexBufferH2M::Create(uint32_t size)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return RefH2M<IndexBufferH2M>();
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLIndexBufferH2M>::Create(size);
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanIndexBufferH2M>::Create(size);
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11IndexBuffer>::Create(size);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<IndexBufferH2M>();
	}

	RefH2M<IndexBufferH2M> IndexBufferH2M::Create(void* data, uint32_t size)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return RefH2M<IndexBufferH2M>();
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLIndexBufferH2M>::Create(data, size);
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanIndexBufferH2M>::Create(data, size);
			case RendererAPITypeH2M::DX11:    return RefH2M<DX11IndexBuffer>::Create(data, size);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<IndexBufferH2M>();
	}

}
