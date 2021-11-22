#include "DX11RenderPass.h"

#include "Hazel/Renderer/HazelFramebuffer.h"


DX11RenderPass::DX11RenderPass(const Hazel::RenderPassSpecification& spec)
	: m_Specification(spec)
{
}

DX11RenderPass::~DX11RenderPass()
{
}
