/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/RenderPassH2M.h"

#include "VulkanH2M.h"


namespace H2M {

	class VulkanRenderPassH2M : public RenderPassH2M
	{
	public:
		VulkanRenderPassH2M(const RenderPassSpecificationH2M& spec);
		virtual ~VulkanRenderPassH2M();

		virtual RenderPassSpecificationH2M& GetSpecification() override { return m_Specification; }
		virtual const RenderPassSpecificationH2M& GetSpecification() const override { return m_Specification; }

	private:
		RenderPassSpecificationH2M m_Specification;
	};

}
