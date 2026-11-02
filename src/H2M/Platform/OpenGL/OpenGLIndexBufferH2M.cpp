/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLIndexBufferH2M.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <GL/glew.h>


namespace H2M {

	//////////////////////////////////////////////////////////////////////////////////
	// IndexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	OpenGLIndexBufferH2M::OpenGLIndexBufferH2M(void* data, uint32_t size)
		: m_RendererID(0), m_Size(size)
	{
		m_LocalData = BufferH2M::Copy(data, size);

		//	Ref<OpenGLIndexBuffer> instance = this;
		//	RendererH2M::Submit([instance]() mutable {
		//		glCreateBuffers(1, &instance->m_RendererID);
		//		glNamedBufferData(instance->m_RendererID, instance->m_Size, instance->m_LocalData.Data, GL_STATIC_DRAW);
		//	});

		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, m_Size, m_LocalData.Data, GL_STATIC_DRAW);
	}

	OpenGLIndexBufferH2M::OpenGLIndexBufferH2M(uint32_t size)
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

	OpenGLIndexBufferH2M::~OpenGLIndexBufferH2M()
	{
		GLuint rendererID = m_RendererID;
		RendererH2M::Submit([rendererID]() {
			glDeleteBuffers(1, &rendererID);
		});
	}

	void OpenGLIndexBufferH2M::SetData(void* data, uint32_t size, uint32_t offset)
	{
		m_LocalData = BufferH2M::Copy(data, size);
		m_Size = size;
		RefH2M<OpenGLIndexBufferH2M> instance = this;
		RendererH2M::Submit([instance, offset]() {
			glNamedBufferSubData(instance->m_RendererID, offset, instance->m_Size, instance->m_LocalData.Data);
		});
	}

	void OpenGLIndexBufferH2M::Bind() const
	{
		RefH2M<const OpenGLIndexBufferH2M> instance = this;
		RendererH2M::Submit([instance]() {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, instance->m_RendererID);
		});
	}

}
