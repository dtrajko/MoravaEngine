#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"

#include "H2M/Renderer/RenderPass.h"


class DX11RenderPass : public H2M::RenderPass
{
public:
	DX11RenderPass(const H2M::RenderPassSpecification& spec);
	virtual ~DX11RenderPass();

	virtual H2M::RenderPassSpecification& GetSpecification() override { return m_Specification; }
	virtual const H2M::RenderPassSpecification& GetSpecification() const override { return m_Specification; }

private:
	H2M::RenderPassSpecification m_Specification;

};
