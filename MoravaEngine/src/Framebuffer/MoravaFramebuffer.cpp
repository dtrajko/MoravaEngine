#include "Framebuffer/MoravaFramebuffer.h"

#include "Platform/OpenGL/OpenGLMoravaFramebuffer.h"
#include "Platform/Vulkan/VulkanMoravaFramebuffer.h"
#include "Platform/DX11/DX11MoravaFramebuffer.h"


H2M::Ref<MoravaFramebuffer> MoravaFramebuffer::Create(uint32_t width, uint32_t height)
{
	switch (H2M::RendererAPIH2M::Current())
	{
		case H2M::RendererAPIH2MType::None: return H2M::Ref<MoravaFramebuffer>();
		case H2M::RendererAPIH2MType::OpenGL: return H2M::Ref<OpenGLMoravaFramebuffer>::Create(width, height);
		case H2M::RendererAPIH2MType::Vulkan: return H2M::Ref<VulkanMoravaFramebuffer>::Create(width, height);
		case H2M::RendererAPIH2MType::DX11: return H2M::Ref<DX11MoravaFramebuffer>::Create(width, height);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::Ref<MoravaFramebuffer>();
}

H2M::Ref<MoravaFramebuffer> MoravaFramebuffer::Create(FramebufferSpecification spec)
{
	switch (H2M::RendererAPIH2M::Current())
	{
		case H2M::RendererAPIH2MType::None: return H2M::Ref<MoravaFramebuffer>();
		case H2M::RendererAPIH2MType::OpenGL: return H2M::Ref<OpenGLMoravaFramebuffer>::Create(spec);
		case H2M::RendererAPIH2MType::Vulkan: return H2M::Ref<VulkanMoravaFramebuffer>::Create(spec);
		case H2M::RendererAPIH2MType::DX11: return H2M::Ref<DX11MoravaFramebuffer>::Create(spec);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::Ref<MoravaFramebuffer>();
}
