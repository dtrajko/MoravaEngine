#include "Framebuffer/MoravaFramebuffer.h"

#include "Platform/OpenGL/OpenGLMoravaFramebuffer.h"
#include "Platform/Vulkan/VulkanMoravaFramebuffer.h"
#include "Platform/DX11/DX11MoravaFramebuffer.h"


Hazel::Ref<MoravaFramebuffer> MoravaFramebuffer::Create(uint32_t width, uint32_t height)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None: return Hazel::Ref<MoravaFramebuffer>();
		case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<OpenGLMoravaFramebuffer>::Create(width, height);
		case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<VulkanMoravaFramebuffer>::Create(width, height);
		case Hazel::RendererAPIType::DX11: return Hazel::Ref<DX11MoravaFramebuffer>::Create(width, height);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return Hazel::Ref<MoravaFramebuffer>();
}

Hazel::Ref<MoravaFramebuffer> MoravaFramebuffer::Create(FramebufferSpecification spec)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None: return Hazel::Ref<MoravaFramebuffer>();
		case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<OpenGLMoravaFramebuffer>::Create(spec);
		case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<VulkanMoravaFramebuffer>::Create(spec);
		case Hazel::RendererAPIType::DX11: return Hazel::Ref<DX11MoravaFramebuffer>::Create(spec);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return Hazel::Ref<MoravaFramebuffer>();
}
