#include "Framebuffer/MoravaFramebuffer.h"

#include "Platform/OpenGL/OpenGLMoravaFramebuffer.h"
#include "Platform/Vulkan/VulkanMoravaFramebuffer.h"
#include "Platform/DX11/DX11MoravaFramebuffer.h"


H2M::RefH2M<MoravaFramebuffer> MoravaFramebuffer::Create(uint32_t width, uint32_t height)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None: return H2M::RefH2M<MoravaFramebuffer>();
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaFramebuffer>::Create(width, height);
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaFramebuffer>::Create(width, height);
		case H2M::RendererAPITypeH2M::DX11: return H2M::RefH2M<DX11MoravaFramebuffer>::Create(width, height);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaFramebuffer>();
}

H2M::RefH2M<MoravaFramebuffer> MoravaFramebuffer::Create(FramebufferSpecification spec)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None: return H2M::RefH2M<MoravaFramebuffer>();
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaFramebuffer>::Create(spec);
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaFramebuffer>::Create(spec);
		case H2M::RendererAPITypeH2M::DX11: return H2M::RefH2M<DX11MoravaFramebuffer>::Create(spec);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaFramebuffer>();
}
