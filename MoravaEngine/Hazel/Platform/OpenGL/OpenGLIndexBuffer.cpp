#include "OpenGLIndexBuffer.h"

#include "../../Core/Assert.h"

#include <gl/glew.h>


namespace Hazel {


	//////////////////////////////////////////////////////////////////////////////////
	// IndexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLIndexBuffer::OpenGLIndexBuffer(void* data, uint32_t size)
		: m_RendererID(0), m_Size(size)
	{
		m_LocalData = Buffer::Copy(data, size);

		//	Ref<OpenGLIndexBuffer> instance = this;
		//	Renderer::Submit([instance]() mutable {
		//		glCreateBuffers(1, &instance->m_RendererID);
		//		glNamedBufferData(instance->m_RendererID, instance->m_Size, instance->m_LocalData.Data, GL_STATIC_DRAW);
		//	});

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, m_LocalData.Data, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size)
		: m_Size(size)
	{
		// m_LocalData = Buffer(size);

		//	Ref<OpenGLIndexBuffer> instance = this;
		//	Renderer::Submit([instance]() mutable {
		//		glCreateBuffers(1, &instance->m_RendererID);
		//		glNamedBufferData(instance->m_RendererID, instance->m_Size, nullptr, GL_DYNAMIC_DRAW);
		//	});

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, nullptr, GL_DYNAMIC_DRAW);
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
