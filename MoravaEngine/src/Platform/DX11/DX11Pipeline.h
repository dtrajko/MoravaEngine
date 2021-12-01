#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "DX11Shader.h"
#include "H2M/Renderer/Pipeline.h"


class DX11Pipeline : public H2M::Pipeline
{
public:
	DX11Pipeline(const H2M::PipelineSpecification& spec);
	virtual ~DX11Pipeline();

	virtual H2M::PipelineSpecification& GetSpecification() { return m_Specification; }
	virtual const H2M::PipelineSpecification& GetSpecification() const { return m_Specification; }

	virtual void Invalidate() override;
	virtual void SetUniformBuffer(H2M::RefH2M<H2M::UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0) override;

	virtual void Bind() override;

	inline ID3D11InputLayout* GetInputLayout() { return m_InputLayout; }

private:
	H2M::PipelineSpecification m_Specification;

	ID3D11InputLayout* m_InputLayout;

};
