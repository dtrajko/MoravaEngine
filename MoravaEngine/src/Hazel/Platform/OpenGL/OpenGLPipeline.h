#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/Pipeline.h"


namespace Hazel {

	class OpenGLPipeline : public Pipeline
	{
	public:
		OpenGLPipeline(const PipelineSpecification& spec);
		virtual ~OpenGLPipeline();

		virtual PipelineSpecification& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;

		// TEMP: remove this when render command buffers are a thing
		virtual void Bind() override;

	private:
		PipelineSpecification m_Specification;
		uint32_t m_VertexArrayRendererID = 0;

	};

}
