#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "H2M/Renderer/RenderPassH2M.h"


class DX11RenderPass : public H2M::RenderPassH2M
{
public:
	DX11RenderPass(const H2M::RenderPassSpecificationH2M& spec);
	virtual ~DX11RenderPass();

	virtual H2M::RenderPassSpecificationH2M& GetSpecification() override { return m_Specification; }
	virtual const H2M::RenderPassSpecificationH2M& GetSpecification() const override { return m_Specification; }

private:
	H2M::RenderPassSpecificationH2M m_Specification;

};
