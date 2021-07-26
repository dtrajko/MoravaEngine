#pragma once

#include "Core/CommonValues.h"

#include "Hazel/Renderer/HazelTexture.h"

#include <string>


class MoravaTexture : public Hazel::HazelTexture2D
{
public:
	struct Specification
	{
		int InternalFormat;
		uint32_t Width;
		uint32_t Height;
		int Border;
		uint32_t Format;
		uint32_t Type;
		int Texture_Wrap_S;
		int Texture_Wrap_T;
		int Texture_Wrap_R;
		int Texture_Min_Filter;
		int Texture_Mag_Filter;
		int MipLevel;
		bool FlipVertically;
		int BitDepth;
		bool IsSampler; // m_Buffer is required during object lifetime, so it must be deallocated in destructor
		uint32_t Samples;
		bool IsMultisample;
		bool IsSRGB;
	} m_Spec;

public:
	static Hazel::Ref<MoravaTexture> Create(const char* fileLoc, bool flipVert = false, bool isSampler = false, int filter = 0);
	static Hazel::Ref<MoravaTexture> Create(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter);
	static Hazel::Ref<MoravaTexture> Create(const char* fileLoc, Specification spec);

	virtual bool Load(bool flipVert = false) = 0;
	virtual void CreateAPISpecific() = 0;
	virtual void Save() = 0;
	virtual Hazel::TextureFormat GetFormat() = 0;

	// BEGIN pure virtual methods inherited from HazelTexture/HazelTexture2D
	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage() const = 0;
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
	virtual Hazel::Buffer GetWriteableBuffer() = 0;
	virtual void Resize(uint32_t width, uint32_t height) = 0;
	virtual bool Loaded() const = 0;
	virtual const std::string& GetPath() const = 0;
	virtual void Bind(uint32_t textureSlot = 0) const = 0;
	virtual Hazel::HazelImageFormat GetFormat() const = 0;
	virtual uint32_t GetMipLevelCount() const = 0;
	virtual uint64_t GetHash() const = 0;
	virtual bool operator==(const Hazel::HazelTexture& other) const = 0;
	virtual Hazel::RendererID GetRendererID() const = 0;
	// END pure virtual methods inherited from HazelTexture/HazelTexture2D

	virtual bool IsLoaded() const = 0;

	inline uint32_t GetID() const { return m_ID; };
	inline uint32_t GetWidth() const { return m_Spec.Width; };
	inline uint32_t GetHeight() const { return m_Spec.Height; };

	virtual void Unbind();
	virtual void Clear();
	virtual uint32_t CalculateMipMapCount(uint32_t width, uint32_t height); // used in Hazel::SceneRenderer
	virtual uint32_t GetMipLevelCount();
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const;

	// Getters
	virtual int GetRed(int x, int z);
	virtual int GetGreen(int x, int z);
	virtual int GetBlue(int x, int z);
	virtual int GetAlpha(int x, int z);
	virtual float GetFileSize(const char* filename);

	// Setters
	virtual void SetPixel(int x, int z, glm::ivec4 pixel);
	virtual void SetRed(int x, int z, int value);
	virtual void SetGreen(int x, int z, int value);
	virtual void SetBlue(int x, int z, int value);
	virtual void SetAlpha(int x, int z, int value);

protected:
	uint32_t m_ID;

};
