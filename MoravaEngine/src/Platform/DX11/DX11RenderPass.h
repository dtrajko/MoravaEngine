#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/RenderPass.h"

#include "DX11.h"


class DX11RenderPass : public Hazel::RenderPass
{
public:
	DX11RenderPass(const Hazel::RenderPassSpecification& spec);
	virtual ~DX11RenderPass();

	virtual Hazel::RenderPassSpecification& GetSpecification() override { return m_Specification; }
	virtual const Hazel::RenderPassSpecification& GetSpecification() const override { return m_Specification; }

private:
	Hazel::RenderPassSpecification m_Specification;

};
