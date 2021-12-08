/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/PipelineH2M.h"


namespace H2M
{

	class OpenGLPipelineH2M : public PipelineH2M
	{
	public:
		OpenGLPipelineH2M(const PipelineSpecificationH2M& spec);
		virtual ~OpenGLPipelineH2M();

		virtual PipelineSpecificationH2M& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecificationH2M& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;

		// TEMP: remove this when render command buffers are a thing
		virtual void Bind() override;

	private:
		PipelineSpecificationH2M m_Specification;
		uint32_t m_VertexArrayRendererID = 0;

	};

}
