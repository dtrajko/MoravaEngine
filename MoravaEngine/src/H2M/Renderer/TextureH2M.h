#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Asset/AssetH2M.h"
#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/BufferH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/ImageH2M.h"
#include "H2M/Renderer/RendererTypesH2M.h"


typedef void* ImTextureID;


namespace H2M
{

	enum class TextureFormatH2M
	{
		None    = 0,
		RGB     = 1,
		RGBA    = 2,
		Float16 = 3
	};

	enum class TextureWrapH2M
	{
		None   = 0,
		Clamp  = 1,
		Repeat = 2
	};

	enum class TextureTypeH2M
	{
		None = 0,
		Texture2D,
		TextureCube
	};

	class TextureH2M : public AssetH2M
	{
	public:
		virtual ~TextureH2M() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual ImageFormatH2M GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const = 0;

		virtual uint64_t GetHash() const = 0;

		virtual TextureTypeH2M GetType() const = 0;

		virtual bool operator==(const TextureH2M& other) const = 0;

		virtual uint32_t GetID() const = 0;

		// virtual RendererID_H2M GetRendererID() const = 0; // Removed in Hazel Live 18.03.2021 #2
		// static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height); // Removed in Hazel Live 18.03.2021 #2

		virtual void SetData(void* data, uint32_t size);
		static uint32_t GetBPP(ImageFormatH2M format);
	};

	class Texture2D_H2M : public TextureH2M
	{
	public:
		static RefH2M<Texture2D_H2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
		static RefH2M<Texture2D_H2M> Create(const std::string& path, bool srgb = false);
		// static RefH2M<Texture2D_H2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrapH2M::Clamp);

		virtual RefH2M<Image2D_H2M> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual BufferH2M GetWriteableBuffer() = 0;

		// virtual void Resize(uint32_t width, uint32_t height) = 0; // method removed in Hazel Live 18.03.2021 #2

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual TextureTypeH2M GetType() const override { return TextureTypeH2M::Texture2D; }

		virtual ImTextureID GetImTextureID();
	};

	class TextureCubeH2M : public TextureH2M
	{
	public:
		static RefH2M<TextureCubeH2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
		static RefH2M<TextureCubeH2M> Create(const std::string& path);

		// Used only by the OpenGL EnvMap scene. Scheduled for removal.
		static RefH2M<TextureCubeH2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed);

		virtual const std::string& GetPath() const = 0;

		virtual TextureTypeH2M GetType() const override { return TextureTypeH2M::TextureCube; }

		// temp debug variable
		bool m_MipsGenerated = false;

	};

}
