#include "IndexBuffer.h"

#include "../Platform/OpenGL/OpenGLIndexBuffer.h"


namespace Hazel {

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::RendererAPIType::None:    return Ref<IndexBuffer>();
			case RendererAPI::RendererAPIType::OpenGL:  return Ref<OpenGLIndexBuffer>::Create(size);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<IndexBuffer>();
	}

	Ref<IndexBuffer> IndexBuffer::Create(void* data, uint32_t size)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::RendererAPIType::None:    return Ref<IndexBuffer>();
			case RendererAPI::RendererAPIType::OpenGL:  return Ref<OpenGLIndexBuffer>::Create(data, size);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<IndexBuffer>();
	}

}
