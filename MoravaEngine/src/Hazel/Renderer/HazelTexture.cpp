#include "HazelTexture.h"

#include "RendererAPI.h"
#include "../Platform/OpenGL/OpenGLTexture.h"


namespace Hazel {

	Ref<HazelTexture2D> HazelTexture2D::Create(HazelTextureFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(format, width, height, wrap);
		}
		return nullptr;
	}

	Ref<HazelTexture2D> HazelTexture2D::Create(const std::string& path, bool srgb, HazelTextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(path, srgb, wrap);
		}
		return nullptr;
	}

	Ref<HazelTextureCube> HazelTextureCube::Create(HazelTextureFormat format, uint32_t width, uint32_t height)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<OpenGLTextureCube>::Create(format, width, height);
		}
		return nullptr;
	}

	Ref<HazelTextureCube> HazelTextureCube::Create(const std::string& path)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<OpenGLTextureCube>::Create(path);
		}
		return nullptr;
	}

	void HazelTexture::SetData(void* data, uint32_t size)
	{
	}

	uint32_t HazelTexture::GetBPP(HazelTextureFormat format)
	{
		switch (format)
		{
			case HazelTextureFormat::RGB:    return 3;
			case HazelTextureFormat::RGBA:   return 4;
		}
		return 0;
	}

	uint32_t HazelTexture::CalculateMipMapCount(uint32_t width, uint32_t height)
	{
		uint32_t levels = 1;
		while ((width | height) >> levels)
			levels++;

		return levels;
	}

}
