#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Renderer/RenderPass.h"

#include "Framebuffer/Framebuffer.h"


struct RenderPassSpecificationHazelVulkan : public Hazel::RenderPassSpecification
{
	Hazel::Ref<Framebuffer> TargetFramebuffer;
	std::string DebugName;
};

class RenderPassHazelVulkan : public Hazel::RenderPass
{
public:
	RenderPassHazelVulkan(const RenderPassSpecificationHazelVulkan& spec);
	virtual ~RenderPassHazelVulkan() = default;

	virtual RenderPassSpecificationHazelVulkan& GetSpecification() override { return m_Specification; }
	virtual const RenderPassSpecificationHazelVulkan& GetSpecification() const override { return m_Specification; }

	static Hazel::Ref<RenderPassHazelVulkan> Create(const RenderPassSpecificationHazelVulkan& spec);

private:
	RenderPassSpecificationHazelVulkan m_Specification;

};
