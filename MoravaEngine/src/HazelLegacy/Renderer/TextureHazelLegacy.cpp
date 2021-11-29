#include "TextureHazelLegacy.h"

#include "Hazel/Platform/OpenGL/OpenGLTexture.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Util.h"
#include "Platform/DX11/DX11Texture2D.h"
#include "Platform/DX11/DX11TextureCube.h"

#include "imgui.h"


namespace HazelLegacy {

	Hazel::Ref<Texture2DHazelLegacy> Texture2DHazelLegacy::Create(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data, Hazel::TextureProperties properties)
	{
		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<Texture2DHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<Hazel::OpenGLTexture2D>::Create(format, width, height, data);
			case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<Hazel::VulkanTexture2D>::Create(format, width, height, data, properties);
			case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11Texture2D>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Hazel::Ref<Texture2DHazelLegacy>();
	}

	Hazel::Ref<Texture2DHazelLegacy> Texture2DHazelLegacy::Create(const std::string& path, Hazel::TextureProperties properties)
	{
		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<Texture2DHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<Hazel::OpenGLTexture2D>::Create(path, properties);
			case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<Hazel::VulkanTexture2D>::Create(path, properties);
			case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11Texture2D>::Create(Util::to_wstr(path.c_str()).c_str());
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Hazel::Ref<Texture2DHazelLegacy>();
	}

	/**** BEGIN Method removed in Vulkan branch ****
	Ref<Texture2DHazelLegacy> Texture2DHazelLegacy::Create(HazelImageFormat format, uint32_t width, uint32_t height, TextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:   return Ref<Texture2DHazelLegacy>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::DX11:   return Ref<DX11Texture2D>::Create(format, width, height, wrap);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<Texture2DHazelLegacy>();
	}
	/**** END Method removed in Vulkan branch ****/

	ImTextureID Texture2DHazelLegacy::GetImTextureID()
	{
		return (ImTextureID)(intptr_t)GetID();
	}

	Hazel::Ref<TextureCubeHazelLegacy> TextureCubeHazelLegacy::Create(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<TextureCubeHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<Hazel::OpenGLTextureCube>::Create(format, width, height, data);
			case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<Hazel::VulkanTextureCube>::Create(format, width, height, data, Hazel::TextureProperties{});
			case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11TextureCube>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Hazel::Ref<TextureCubeHazelLegacy>();
	}

	Hazel::Ref<TextureCubeHazelLegacy> TextureCubeHazelLegacy::Create(const std::string& path)
	{
		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<TextureCubeHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<Hazel::OpenGLTextureCube>::Create(path);
			case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<Hazel::VulkanTextureCube>::Create(path, Hazel::TextureProperties{});
			case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11TextureCube>::Create(path);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Hazel::Ref<TextureCubeHazelLegacy>();
	}

	// Used only by the OpenGL EnvMap scene. Scheduled for removal.
	Hazel::Ref<TextureCubeHazelLegacy> TextureCubeHazelLegacy::Create(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, bool notUsed)
	{
		switch (Hazel::RendererAPI::Current())
		{
			case Hazel::RendererAPIType::None:   return Hazel::Ref<TextureCubeHazelLegacy>();
			case Hazel::RendererAPIType::OpenGL: return Hazel::Ref<Hazel::OpenGLTextureCube>::Create(format, width, height, true);
			case Hazel::RendererAPIType::Vulkan: return Hazel::Ref<Hazel::VulkanTextureCube>::Create(format, width, height, nullptr, Hazel::TextureProperties{});
			case Hazel::RendererAPIType::DX11:   return Hazel::Ref<DX11TextureCube>::Create(format, width, height, nullptr);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Hazel::Ref<TextureCubeHazelLegacy>();
	}

	void TextureHazelLegacy::SetData(void* data, uint32_t size)
	{
	}

	uint32_t TextureHazelLegacy::GetBPP(Hazel::HazelImageFormat format)
	{
		switch (format)
		{
			case Hazel::HazelImageFormat::RGB:    return 3;
			case Hazel::HazelImageFormat::RGBA:   return 4;
		}
		return 0;
	}

}
