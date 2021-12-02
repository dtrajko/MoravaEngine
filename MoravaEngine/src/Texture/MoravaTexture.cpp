#include "Texture/MoravaTexture.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/RendererAPI.h"

#include "Platform/OpenGL/OpenGLMoravaTexture.h"
#include "Platform/Vulkan/VulkanMoravaTexture.h"
#include "Platform/DX11/DX11MoravaTexture.h"


H2M::RefH2M<MoravaTexture> MoravaTexture::Create(const char* fileLoc, bool flipVert, bool isSampler, int filter)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<MoravaTexture>();
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaTexture>::Create(fileLoc, flipVert, isSampler, filter);
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaTexture>::Create(fileLoc, flipVert, isSampler, filter);
		case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11MoravaTexture>::Create(fileLoc, flipVert, isSampler, filter);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaTexture>();
}

H2M::RefH2M<MoravaTexture> MoravaTexture::Create(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<MoravaTexture>();
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaTexture>::Create(fileLoc, width, height, isSampler, filter);
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaTexture>::Create(fileLoc, width, height, isSampler, filter);
		case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11MoravaTexture>::Create(fileLoc, width, height, isSampler, filter);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaTexture>();
}

H2M::RefH2M<MoravaTexture> MoravaTexture::Create(const char* fileLoc, Specification spec)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
		case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<MoravaTexture>();
		case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<OpenGLMoravaTexture>::Create(fileLoc, spec);
		case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<VulkanMoravaTexture>::Create(fileLoc, spec);
		case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11MoravaTexture>::Create(fileLoc, spec);
	}

	Log::GetLogger()->error("Unknown RendererAPI");
	H2M_CORE_ASSERT(false, "Unknown RendererAPI");
	return H2M::RefH2M<MoravaTexture>();
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
