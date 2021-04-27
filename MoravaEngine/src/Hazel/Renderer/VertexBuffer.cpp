#include "VertexBuffer.h"

#include "../Platform/OpenGL/OpenGLVertexBuffer.h"


namespace Hazel {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::RendererAPIType::None:    return Ref<VertexBuffer>();
		case RendererAPI::RendererAPIType::OpenGL:  return Ref<OpenGLVertexBuffer>::Create(data, size, usage);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<VertexBuffer>();
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::RendererAPIType::None:    return Ref<VertexBuffer>();
		case RendererAPI::RendererAPIType::OpenGL:  return Ref<OpenGLVertexBuffer>::Create(size, usage);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<VertexBuffer>();
	}

}
