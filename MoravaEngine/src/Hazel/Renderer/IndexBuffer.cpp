#include "IndexBuffer.h"

#include "Hazel/Platform/OpenGL/OpenGLIndexBuffer.h"
#include "Hazel/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Platform/DX11/DX11IndexBuffer.h"
#include "Hazel/Renderer/HazelMesh.h"


namespace Hazel {

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return Ref<IndexBuffer>();
			case RendererAPIType::OpenGL:  return Ref<OpenGLIndexBuffer>::Create(size);
			case RendererAPIType::Vulkan:  return Ref<VulkanIndexBuffer>::Create(size);
			case RendererAPIType::DX11:    return Ref<DX11IndexBuffer>::Create(size);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<IndexBuffer>();
	}

	Ref<IndexBuffer> IndexBuffer::Create(void* data, uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return Ref<IndexBuffer>();
			case RendererAPIType::OpenGL:  return Ref<OpenGLIndexBuffer>::Create(data, size);
			case RendererAPIType::Vulkan:  return Ref<VulkanIndexBuffer>::Create(data, size);
			case RendererAPIType::DX11:    return Ref<DX11IndexBuffer>::Create(data, size / (uint32_t)sizeof(Index));
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<IndexBuffer>();
	}

}
