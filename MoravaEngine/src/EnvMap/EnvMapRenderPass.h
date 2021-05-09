#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Renderer/RenderPass.h"

#include "Framebuffer/Framebuffer.h"


struct EnvMapRenderPassSpecification : public Hazel::RenderPassSpecification
{
	Hazel::Ref<Framebuffer> TargetFramebuffer;
	std::string DebugName;
};

class EnvMapRenderPass : public Hazel::RenderPass
{
public:
	EnvMapRenderPass(const EnvMapRenderPassSpecification& spec);
	virtual ~EnvMapRenderPass() = default;

	virtual EnvMapRenderPassSpecification& GetSpecification() override { return m_Specification; }
	virtual const EnvMapRenderPassSpecification& GetSpecification() const override { return m_Specification; }

	static Hazel::Ref<EnvMapRenderPass> Create(const EnvMapRenderPassSpecification& spec);

private:
	EnvMapRenderPassSpecification m_Specification;

};
