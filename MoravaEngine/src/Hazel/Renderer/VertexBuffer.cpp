#include "VertexBuffer.h"

#include "Hazel/Platform/OpenGL/OpenGLVertexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/DX11/DX11VertexBuffer.h"
#include "Hazel/Renderer/RendererAPI.h"


namespace Hazel {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return Ref<VertexBuffer>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLVertexBuffer>::Create(data, size, usage);
		case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(data, size, usage);
		case RendererAPIType::DX11:    return Ref<DX11VertexBuffer>::Create(data, size, usage);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<VertexBuffer>();
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return Ref<VertexBuffer>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLVertexBuffer>::Create(size, usage);
		case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(size, usage);
		case RendererAPIType::DX11:    return Ref<DX11VertexBuffer>::Create(size, usage);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<VertexBuffer>();
	}

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t stride, uint32_t count, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    return Ref<VertexBuffer>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLVertexBuffer>::Create(data, stride * count, usage);
		case RendererAPIType::Vulkan:  return Ref<VulkanVertexBuffer>::Create(data, stride * count, usage);
		case RendererAPIType::DX11:    return Ref<DX11VertexBuffer>::Create(data, stride, count);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<VertexBuffer>();
	}

}
