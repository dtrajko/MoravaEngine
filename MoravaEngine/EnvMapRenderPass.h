#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/RenderPass.h"

#include "Framebuffer.h"


struct EnvMapRenderPassSpecification : public Hazel::RenderPassSpecification
{
	Ref<Framebuffer> TargetFramebuffer;
};

class EnvMapRenderPass : public Hazel::RenderPass
{
public:
	virtual ~EnvMapRenderPass() = default;

	virtual EnvMapRenderPassSpecification& GetSpecification() override { return m_Specification; }
	virtual const EnvMapRenderPassSpecification& GetSpecification() const override { return m_Specification; }

	static Hazel::Ref<EnvMapRenderPass> Create(const Hazel::RenderPassSpecification& spec);

private:
	EnvMapRenderPassSpecification m_Specification;

};
