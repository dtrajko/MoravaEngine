#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/RenderPass.h"

#include "Vulkan.h"


namespace Hazel {

	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual RenderPassSpecification& GetSpecification() override { return m_Specification; }
		virtual const RenderPassSpecification& GetSpecification() const override { return m_Specification; }

	private:
		RenderPassSpecification m_Specification;
	};

}
