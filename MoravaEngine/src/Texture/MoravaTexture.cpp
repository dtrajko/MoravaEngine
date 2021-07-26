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

void MoravaTexture::Unbind()
{
	Log::GetLogger()->error("Method MoravaTexture::Unbind() should not be called directly!");
}

void MoravaTexture::Clear()
{
	Log::GetLogger()->error("Method MoravaTexture::Clear() should not be called directly!");
}

uint32_t MoravaTexture::CalculateMipMapCount(uint32_t width, uint32_t height)
{
	Log::GetLogger()->error("Method MoravaTexture::CalculateMipMapCount() should not be called directly!");
	return uint32_t();
}

uint32_t MoravaTexture::GetMipLevelCount()
{
	Log::GetLogger()->error("Method MoravaTexture::GetMipLevelCount() should not be called directly!");
	return uint32_t();
}

std::pair<uint32_t, uint32_t> MoravaTexture::GetMipSize(uint32_t mip) const
{
	Log::GetLogger()->error("Method MoravaTexture::GetMipSize() should not be called directly!");
	return std::pair<uint32_t, uint32_t>();
}

int MoravaTexture::GetRed(int x, int z)
{
	Log::GetLogger()->error("Method MoravaTexture::GetRed() should not be called directly!");
	return 0;
}

int MoravaTexture::GetGreen(int x, int z)
{
	Log::GetLogger()->error("Method MoravaTexture::GetGreen() should not be called directly!");
	return 0;
}

int MoravaTexture::GetBlue(int x, int z)
{
	Log::GetLogger()->error("Method MoravaTexture::GetBlue() should not be called directly!");
	return 0;
}

int MoravaTexture::GetAlpha(int x, int z)
{
	Log::GetLogger()->error("Method MoravaTexture::GetAlpha() should not be called directly!");
	return 0;
}

float MoravaTexture::GetFileSize(const char* filename)
{
	Log::GetLogger()->error("Method MoravaTexture::GetFileSize() should not be called directly!");
	return 0.0f;
}

void MoravaTexture::SetPixel(int x, int z, glm::ivec4 pixel)
{
	Log::GetLogger()->error("Method MoravaTexture::SetPixel() should not be called directly!");
}

void MoravaTexture::SetRed(int x, int z, int value)
{
	Log::GetLogger()->error("Method MoravaTexture::SetRed() should not be called directly!");
}

void MoravaTexture::SetGreen(int x, int z, int value)
{
	Log::GetLogger()->error("Method MoravaTexture::SetGreen() should not be called directly!");
}

void MoravaTexture::SetBlue(int x, int z, int value)
{
	Log::GetLogger()->error("Method MoravaTexture::SetBlue() should not be called directly!");
}

void MoravaTexture::SetAlpha(int x, int z, int value)
{
	Log::GetLogger()->error("Method MoravaTexture::SetAlpha() should not be called directly!");
}
