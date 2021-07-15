#include "Texture/MoravaTexture.h"

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Platform/OpenGL/OpenGLMoravaTexture.h"
#include "Platform/Vulkan/VulkanMoravaTexture.h"
#include "Platform/DX11/DX11MoravaTexture.h"


Hazel::Ref<MoravaTexture> MoravaTexture::Create(const char* fileLoc, bool flipVert, bool isSampler, int filter)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return Hazel::Ref<MoravaTexture>();
		case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<OpenGLMoravaTexture>::Create(fileLoc, flipVert, isSampler, filter);
		case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<VulkanMoravaTexture>::Create(fileLoc, flipVert, isSampler, filter);
		case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11MoravaTexture>::Create(fileLoc, flipVert, isSampler, filter);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return Hazel::Ref<MoravaTexture>();
}

Hazel::Ref<MoravaTexture> MoravaTexture::Create(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return Hazel::Ref<MoravaTexture>();
		case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<OpenGLMoravaTexture>::Create(fileLoc, width, height, isSampler, filter);
		case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<VulkanMoravaTexture>::Create(fileLoc, width, height, isSampler, filter);
		case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11MoravaTexture>::Create(fileLoc, width, height, isSampler, filter);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return Hazel::Ref<MoravaTexture>();
}

Hazel::Ref<MoravaTexture> MoravaTexture::Create(const char* fileLoc, Specification spec)
{
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::None:   return Hazel::Ref<MoravaTexture>();
		case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<OpenGLMoravaTexture>::Create(fileLoc, spec);
		case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<VulkanMoravaTexture>::Create(fileLoc, spec);
		case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11MoravaTexture>::Create(fileLoc, spec);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	HZ_CORE_ASSERT(false, "Unknown RendererAPI");
	return Hazel::Ref<MoravaTexture>();
}
