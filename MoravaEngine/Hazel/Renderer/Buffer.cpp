#include "Buffer.h"

#include "../Platform/OpenGL/OpenGLBuffer.h"


namespace Hazel {

	VertexBuffer* VertexBuffer::Create(void* data, uint32_t size, VertexBufferUsage usage)
	{
		return (VertexBuffer*)new OpenGLVertexBuffer(data, size, usage);
	}

	VertexBuffer* VertexBuffer::Create(uint32_t size, VertexBufferUsage usage)
	{
		return (VertexBuffer*)new OpenGLVertexBuffer(size, usage);
	}

	IndexBuffer* IndexBuffer::Create(void* data, uint32_t size)
	{
		return (IndexBuffer*)new OpenGLIndexBuffer(data, size);
	}

}
