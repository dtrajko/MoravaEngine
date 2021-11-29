#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Asset/Asset.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Core/Buffer.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/RendererTypes.h"

#include "HazelLegacy/Renderer/ImageHazelLegacy.h"


typedef void* ImTextureID;


namespace HazelLegacy {

	class TextureHazelLegacy : public Hazel::Asset
	{
	public:
		virtual ~TextureHazelLegacy() {}

		virtual void Bind(uint32_t slot = 0) const = 0;

		virtual ImageFormatHazelLegacy GetFormat() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetMipLevelCount() const = 0;
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const = 0;

		virtual uint64_t GetHash() const = 0;

		virtual TextureTypeHazelLegacy GetType() const = 0;

		virtual bool operator==(const TextureHazelLegacy& other) const { return bool{}; };

		virtual uint32_t GetID() const { return uint32_t{}; };

		// virtual RendererID GetRendererID() const = 0; // Removed in Hazel Live 18.03.2021 #2
		// static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height); // Removed in Hazel Live 18.03.2021 #2

		virtual void SetData(void* data, uint32_t size);
		static uint32_t GetBPP(Hazel::HazelImageFormat format);
	};

	class Texture2DHazelLegacy : public TextureHazelLegacy
	{
	public:
		static Hazel::Ref<Texture2DHazelLegacy> Create(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr, Hazel::TextureProperties properties = Hazel::TextureProperties());
		static Hazel::Ref<Texture2DHazelLegacy> Create(const std::string& path, Hazel::TextureProperties properties = Hazel::TextureProperties());

		// static Hazel::Ref<Texture2DHazelLegacy> Create(const std::string& path, bool srgb = false);
		// static Hazel::Ref<Texture2DHazelLegacy> Create(HazelImageFormat format, uint32_t width, uint32_t height, TextureWrap wrap = TextureWrap::Clamp);

		virtual Hazel::Ref<Hazel::HazelImage2D> GetImage() const = 0;

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual Hazel::Buffer GetWriteableBuffer() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual bool Loaded() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual TextureTypeHazelLegacy GetType() const override { return TextureTypeHazelLegacy::Texture2D; }

		virtual ImTextureID GetImTextureID();
	};

	class TextureCubeHazelLegacy : public TextureHazelLegacy
	{
	public:
		static Hazel::Ref<TextureCubeHazelLegacy> Create(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, const void* data = nullptr);
		static Hazel::Ref<TextureCubeHazelLegacy> Create(const std::string& path);

		// Used only by the OpenGL EnvMap scene. Scheduled for removal.
		static Hazel::Ref<TextureCubeHazelLegacy> Create(Hazel::HazelImageFormat format, uint32_t width, uint32_t height, bool notUsed);

		virtual const std::string& GetPath() const { return std::string{}; };

		virtual TextureTypeHazelLegacy GetType() const override { return TextureTypeHazelLegacy::TextureCube; }

		// temp debug variable
		bool m_MipsGenerated = false;

	};

}
