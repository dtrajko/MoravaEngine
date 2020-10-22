#include "HazelTexture.h"
#include "../Platform/OpenGL/OpenGLTexture.h"


namespace Hazel {

	HazelTexture2D* HazelTexture2D::Create(HazelTextureFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap)
	{
		return new OpenGLTexture2D(format, width, height, wrap);
	}

	HazelTexture2D* HazelTexture2D::Create(const std::string& path, bool srgb)
	{
		return new OpenGLTexture2D(path, srgb);
	}

	HazelTextureCube* HazelTextureCube::Create(HazelTextureFormat format, uint32_t width, uint32_t height)
	{
		return new OpenGLTextureCube(format, width, height);
	}

	HazelTextureCube* HazelTextureCube::Create(const std::string& path)
	{
		return new OpenGLTextureCube(path);
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
