#include "HazelTexture.h"

#include "RendererAPI.h"
#include "Hazel/Platform/OpenGL/OpenGLTexture.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Platform/DX11/DX11Texture2D.h"
#include "Core/Util.h"


namespace Hazel {

	Ref<HazelTexture2D> HazelTexture2D::Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return Ref<HazelTexture2D>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(format, width, height, data);
		case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelTexture2D>();
	}

	Ref<HazelTexture2D> HazelTexture2D::Create(HazelImageFormat format, uint32_t width, uint32_t height, HazelTextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return Ref<HazelTexture2D>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(format, width, height, wrap);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelTexture2D>();
	}

	Ref<HazelTexture2D> HazelTexture2D::Create(const std::string& path, bool srgb, HazelTextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:   return Ref<HazelTexture2D>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(path, srgb, wrap);
		case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(path, srgb, wrap);
		case RendererAPIType::DX11:   return Ref<DX11Texture2D>::Create(Util::to_wstr(path.c_str()).c_str());
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelTexture2D>();
	}

	Ref<HazelTextureCube> HazelTextureCube::Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return Ref<HazelTextureCube>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTextureCube>::Create(format, width, height, data);
		case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelTextureCube>();
	}

	Ref<HazelTextureCube> HazelTextureCube::Create(const std::string& path)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return Ref<HazelTextureCube>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTextureCube>::Create(path);
		case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(path);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelTextureCube>();
	}

	// Used only by the OpenGL EnvMap scene. Scheduled for removal.
	Ref<HazelTextureCube> HazelTextureCube::Create(HazelImageFormat format, uint32_t width, uint32_t height, bool notUsed)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return Ref<HazelTextureCube>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTextureCube>::Create(format, width, height, true);
		case RendererAPIType::Vulkan: return Ref<VulkanTextureCube>::Create(format, width, height, nullptr);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelTextureCube>();
	}

	void HazelTexture::SetData(void* data, uint32_t size)
	{
	}

	uint32_t HazelTexture::GetBPP(HazelImageFormat format)
	{
		switch (format)
		{
			case HazelImageFormat::RGB:    return 3;
			case HazelImageFormat::RGBA:   return 4;
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
