#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Core/Buffer.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelImage.h"
#include "Hazel/Renderer/RendererTypes.h"


typedef void* ImTextureID;


namespace Hazel {

	enum class TextureFormat
	{
		None    = 0,
		RGB     = 1,
		RGBA    = 2,
		Float16 = 3
	};

	enum class TextureWrap
	{
		None   = 0,
		Clamp  = 1,
		Repeat = 2
	};

	enum class TextureType
	{
		None = 0,
		Texture2D,
		TextureCube
	};

	class HazelTexture : public Asset
	{
	public:
		virtual ~HazelTexture() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual HazelImageFormat GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const = 0;

		virtual uint64_t GetHash() const = 0;

		virtual TextureType GetType() const = 0;

		virtual bool operator==(const HazelTexture& other) const = 0;

		virtual uint32_t GetID() const = 0;
		virtual RendererID GetRendererID() const = 0;

		virtual void SetData(void* data, uint32_t size);
		static uint32_t GetBPP(HazelImageFormat format);
		static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);
	};

	class HazelTexture2D : public HazelTexture
	{
	public:
		static Ref<HazelTexture2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<HazelTexture2D> Create(HazelImageFormat format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrap::Clamp);
		static Ref<HazelTexture2D> Create(const std::string& path, bool srgb = false, TextureWrap wrap = TextureWrap::Clamp);

		virtual Ref<HazelImage2D> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual Buffer GetWriteableBuffer() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual TextureType GetType() const override { return TextureType::Texture2D; }

		virtual ImTextureID GetImTextureID();

	};

	class HazelTextureCube : public HazelTexture
	{
	public:
		static Ref<HazelTextureCube> Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Ref<HazelTextureCube> Create(const std::string& path);

		// Used only by the OpenGL EnvMap scene. Scheduled for removal.
		static Ref<HazelTextureCube> Create(HazelImageFormat format, uint32_t width, uint32_t height, bool notUsed);

		virtual const std::string& GetPath() const = 0;

		virtual TextureType GetType() const override { return TextureType::TextureCube; }

		// temp debug variable
		bool m_MipsGenerated = false;

	};

}
