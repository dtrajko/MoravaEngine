/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLVertexBufferH2M.h"

#include "H2M/Core/AssertH2M.h"

#include <GL/glew.h>


namespace H2M
{

	//////////////////////////////////////////////////////////////////////////////////
	// VertexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	static GLenum OpenGLUsage(VertexBufferUsageH2M usage)
	{
		switch (usage)
		{
		case VertexBufferUsageH2M::Static:    return GL_STATIC_DRAW;
		case VertexBufferUsageH2M::Dynamic:   return GL_DYNAMIC_DRAW;
		}
		H2M_CORE_ASSERT(false, "Unknown vertex buffer usage");
		return 0;
	}

	OpenGLVertexBufferH2M::OpenGLVertexBufferH2M(void* data, uint32_t size, VertexBufferUsageH2M usage)
		: m_Size(size), m_Usage(usage)
	{
		m_LocalData = BufferH2M::Copy(data, size);

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, m_LocalData.Data, OpenGLUsage(m_Usage));
	}

	OpenGLVertexBufferH2M::OpenGLVertexBufferH2M(uint32_t size, VertexBufferUsageH2M usage)
		: m_Size(size), m_Usage(usage)
	{
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, nullptr, OpenGLUsage(m_Usage));
	}

	OpenGLVertexBufferH2M::~OpenGLVertexBufferH2M()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLVertexBufferH2M::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_LocalData = BufferH2M::Copy(data, size);
		m_Size = size;
		glNamedBufferSubData(m_RendererID, offset, m_Size, m_LocalData.Data);
	}

	void OpenGLVertexBufferH2M::RT_SetData(void* buffer, uint32_t size, uint32_t offset)
	{
	}

	void OpenGLVertexBufferH2M::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
	}

}
