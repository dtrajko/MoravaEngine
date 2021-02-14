#include "OpenGLIndexBuffer.h"

#include "../../Core/Assert.h"
#include "../../Renderer/HazelRenderer.h"

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
		//	HazelRenderer::Submit([instance]() mutable {
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
		//	HazelRenderer::Submit([instance]() mutable {
		//		glCreateBuffers(1, &instance->m_RendererID);
		//		glNamedBufferData(instance->m_RendererID, instance->m_Size, nullptr, GL_DYNAMIC_DRAW);
		//	});

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		GLuint rendererID = m_RendererID;
		HazelRenderer::Submit([rendererID]() {
			glDeleteBuffers(1, &rendererID);
			});
	}

	void OpenGLIndexBuffer::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_LocalData = Buffer::Copy(data, size);
		m_Size = size;
		Ref<OpenGLIndexBuffer> instance = this;
		HazelRenderer::Submit([instance, offset]() {
			glNamedBufferSubData(instance->m_RendererID, offset, instance->m_Size, instance->m_LocalData.Data);
			});
	}

	void OpenGLIndexBuffer::Bind() const
	{
		Ref<const OpenGLIndexBuffer> instance = this;
		HazelRenderer::Submit([instance]() {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->m_RendererID);
			});
	}

}
