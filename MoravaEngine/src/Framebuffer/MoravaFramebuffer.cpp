#include "Framebuffer/MoravaFramebuffer.h"

#if defined(GRAPHICS_API_OPENGL)
#include "Platform/OpenGL/OpenGLMoravaFramebuffer.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "Platform/Vulkan/VulkanMoravaFramebuffer.h"
#endif
#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11MoravaFramebuffer.h"
#endif


H2M::RefH2M<MoravaFramebuffer> MoravaFramebuffer::Create(uint32_t width, uint32_t height)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None: return H2M::RefH2M<MoravaFramebuffer>();
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaFramebuffer>::Create(width, height);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaFramebuffer>::Create(width, height);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11: return H2M::RefH2M<DX11MoravaFramebuffer>::Create(width, height);
#endif
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaFramebuffer>();
}

H2M::RefH2M<MoravaFramebuffer> MoravaFramebuffer::Create(FramebufferSpecification spec)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None: return H2M::RefH2M<MoravaFramebuffer>();
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaFramebuffer>::Create(spec);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaFramebuffer>::Create(spec);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11: return H2M::RefH2M<DX11MoravaFramebuffer>::Create(spec);
#endif
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaFramebuffer>();
}
