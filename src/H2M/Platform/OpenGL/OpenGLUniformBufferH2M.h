/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/UniformBufferH2M.h"


namespace H2M
{

	class OpenGLUniformBufferH2M : public UniformBufferH2M
	{
	public:
		OpenGLUniformBufferH2M(uint32_t size, uint32_t binding);
		virtual ~OpenGLUniformBufferH2M();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual uint32_t GetBinding() const override;

	private:
		uint32_t m_RendererID = 0;

	};

}
