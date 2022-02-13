/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLUniformBufferH2M.h"

#include "Core/Log.h"


namespace H2M
{

	OpenGLUniformBufferH2M::OpenGLUniformBufferH2M(uint32_t size, uint32_t binding)
	{
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW); // TODO: investigate usage hint
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
	}

	OpenGLUniformBufferH2M::~OpenGLUniformBufferH2M()
	{
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLUniformBufferH2M::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}

	void OpenGLUniformBufferH2M::RT_SetData(const void* data, uint32_t size, uint32_t offset)
	{
		glNamedBufferSubData(m_RendererID, offset, size, data);
	}

	uint32_t OpenGLUniformBufferH2M::GetBinding() const
	{
		Log::GetLogger()->error("OpenGLUniformBufferH2M::GetBinding method not implemented!");
		return uint32_t();
	}

}
