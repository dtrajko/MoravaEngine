#pragma once

#include "Texture/MoravaTexture.h"

#include "Core/CommonValues.h"

#include "Hazel/Renderer/HazelTexture.h"

#include <string>


class DX11MoravaTexture : public MoravaTexture
{
public:
	DX11MoravaTexture();
	DX11MoravaTexture(const char* fileLoc, bool flipVert = false, bool isSampler = false, int filter = 0);
	DX11MoravaTexture(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler = false, int filter = 0);
	DX11MoravaTexture(const char* fileLoc, Specification spec); // constructor for fully customizable texture
	virtual ~DX11MoravaTexture();

	virtual bool Load(bool flipVert = false);
	virtual void CreateAPISpecific();
	virtual void Save();
	virtual Hazel::TextureFormat GetFormat() { return m_Format; };

	// BEGIN pure virtual methods inherited from HazelTexture/HazelTexture2D
	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage() const override { return Hazel::Ref<Hazel::HazelImage2D>(); }
	virtual void Lock() override {}
	virtual void Unlock() override {}
	virtual Hazel::Buffer GetWriteableBuffer() override { return Hazel::Buffer(); }
	virtual void Resize(uint32_t width, uint32_t height) override {}
	virtual bool Loaded() const override { return m_Buffer ? true : false; }
	virtual const std::string& GetPath() const override { return std::string(m_FileLocation); }
	virtual void Bind(uint32_t textureSlot = 0) const override;
	virtual Hazel::HazelImageFormat GetFormat() const override { return Hazel::HazelImageFormat(); }
	virtual uint32_t GetMipLevelCount() const override { return uint32_t(); }
	virtual uint64_t GetHash() const override { return uint64_t(); }
	virtual bool operator==(const Hazel::HazelTexture& other) const override { return m_ID == other.GetID(); }
	virtual Hazel::RendererID GetRendererID() const override { return m_ID; }
	// END pure virtual methods inherited from HazelTexture/HazelTexture2D

	virtual bool IsLoaded() const override { return m_Buffer ? true : false; }; // used in Hazel::Mesh

	inline uint32_t GetID() const { return m_ID; };
	inline uint32_t GetWidth() const { return m_Spec.Width; };
	inline uint32_t GetHeight() const { return m_Spec.Height; };

	virtual void Unbind() override;
	virtual void Clear() override;
	virtual uint32_t CalculateMipMapCount(uint32_t width, uint32_t height) override; // used in Hazel::SceneRenderer
	virtual uint32_t GetMipLevelCount() override;
	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	// Getters
	virtual int GetRed(int x, int z) override;
	virtual int GetGreen(int x, int z) override;
	virtual int GetBlue(int x, int z) override;
	virtual int GetAlpha(int x, int z) override;
	virtual float GetFileSize(const char* filename) override;

	// Setters
	virtual void SetPixel(int x, int z, glm::ivec4 pixel) override;
	virtual void SetRed(int x, int z, int value) override;
	virtual void SetGreen(int x, int z, int value) override;
	virtual void SetBlue(int x, int z, int value) override;
	virtual void SetAlpha(int x, int z, int value) override;

protected:
	const char* m_FileLocation;
	unsigned char* m_Buffer;
	int m_Level;
	Hazel::TextureFormat m_Format;

};