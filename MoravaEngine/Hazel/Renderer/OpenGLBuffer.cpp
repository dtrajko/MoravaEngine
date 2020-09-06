#include "OpenGLBuffer.h"
#include "../../Log.h"

#include <gl/glew.h>


namespace Hazel {

	//////////////////////////////////////////////////////////////////////////////////
	// VertexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	static GLenum OpenGLUsage(VertexBufferUsage usage)
	{
		switch (usage)
		{
			case VertexBufferUsage::Static:    return GL_STATIC_DRAW;
			case VertexBufferUsage::Dynamic:   return GL_DYNAMIC_DRAW;
		}
		Log::GetLogger()->error("Unknown vertex buffer usage");
		return 0;
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage)
		: m_Size(size), m_Usage(usage)
	{
		m_LocalData = Buffer::Copy(data, size);
		 
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, m_LocalData.Data, OpenGLUsage(m_Usage));
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size, VertexBufferUsage usage)
		: m_Size(size), m_Usage(usage)
	{
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, nullptr, OpenGLUsage(m_Usage));
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLVertexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_LocalData = Buffer::Copy(data, size);
		m_Size = size;
		glNamedBufferSubData(m_RendererID, offset, m_Size, m_LocalData.Data);
	}

	void OpenGLVertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	}

	//////////////////////////////////////////////////////////////////////////////////
	// IndexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLIndexBuffer::OpenGLIndexBuffer(void* data, uint32_t size)
		: m_RendererID(0), m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, m_LocalData.Data, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLIndexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_LocalData = Buffer::Copy(data, size);
		m_Size = size;
		glNamedBufferSubData(m_RendererID, offset, m_Size, m_LocalData.Data);
	}

	void OpenGLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
	}

}
