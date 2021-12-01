#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Asset/Asset.h"
#include "H2M/Core/Base.h"
#include "H2M/Core/Buffer.h"
#include "H2M/Core/Ref.h"
#include "H2M/Renderer/RendererTypes.h"

#include "H2M/Renderer/ImageH2M.h"


typedef void* ImTextureID;


namespace H2M {

	class TextureH2M : public H2M::Asset
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

		virtual bool operator==(const TextureH2M& other) const { return bool{}; };

		virtual uint32_t GetID() const { return uint32_t{}; };

		// virtual RendererID GetRendererID() const = 0; // Removed in Hazel Live 18.03.2021 #2
		// static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height); // Removed in Hazel Live 18.03.2021 #2

		virtual void SetData(void* data, uint32_t size);
		static uint32_t GetBPP(H2M::ImageFormatH2M format);
	};

	class Texture2DH2M : public TextureH2M
	{
	public:
		static H2M::RefH2M<Texture2DH2M> Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr, H2M::TextureProperties properties = H2M::TextureProperties());
		static H2M::RefH2M<Texture2DH2M> Create(const std::string& path, H2M::TextureProperties properties = H2M::TextureProperties());

		// static H2M::RefH2M<Texture2DH2M> Create(const std::string& path, bool srgb = false);
		// static H2M::RefH2M<Texture2DH2M> Create(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrap::Clamp);

		virtual H2M::RefH2M<H2M::HazelImage2D> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual H2M::Buffer GetWriteableBuffer() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual TextureTypeH2M GetType() const override { return TextureTypeH2M::Texture2D; }

		virtual ImTextureID GetImTextureID();
	};

	class TextureCubeH2M : public TextureH2M
	{
	public:
		static H2M::RefH2M<TextureCubeH2M> Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data = nullptr);
		static H2M::RefH2M<TextureCubeH2M> Create(const std::string& path);

		// Used only by the OpenGL EnvMap scene. Scheduled for removal.
		static H2M::RefH2M<TextureCubeH2M> Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed);

		virtual const std::string& GetPath() const { return std::string{}; };

		virtual TextureTypeH2M GetType() const override { return TextureTypeH2M::TextureCube; }

		// temp debug variable
		bool m_MipsGenerated = false;

	};

}
