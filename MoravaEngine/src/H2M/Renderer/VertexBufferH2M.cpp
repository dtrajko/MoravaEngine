#include "VertexBufferH2M.h"

#include "H2M/Platform/OpenGL/OpenGLVertexBufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanVertexBufferH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/DX11/DX11VertexBuffer.h"


namespace H2M
{

	RefH2M<VertexBufferH2M> VertexBufferH2M::Create(void* data, uint32_t size, VertexBufferUsageH2M usage)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    return RefH2M<VertexBufferH2M>();
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexBufferH2M>::Create(data, size, usage);
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexBufferH2M>::Create(data, size, usage);
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexBuffer>::Create(data, size, usage);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<VertexBufferH2M>();
	}

	RefH2M<VertexBufferH2M> VertexBufferH2M::Create(uint32_t size, VertexBufferUsageH2M usage)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    return RefH2M<VertexBufferH2M>();
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexBufferH2M>::Create(size, usage);
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexBufferH2M>::Create(size, usage);
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexBuffer>::Create(size, usage);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<VertexBufferH2M>();
	}

	RefH2M<VertexBufferH2M> VertexBufferH2M::Create(void* data, uint32_t stride, uint32_t count, VertexBufferUsageH2M usage)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    return RefH2M<VertexBufferH2M>();
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexBufferH2M>::Create(data, stride * count, usage);
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexBufferH2M>::Create(data, stride * count, usage);
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexBuffer>::Create(data, stride, count);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<VertexBufferH2M>();
	}

}
