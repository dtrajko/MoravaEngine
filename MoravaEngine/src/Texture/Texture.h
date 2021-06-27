#pragma once

#include "Core/CommonValues.h"

#include "Hazel/Renderer/HazelTexture.h"

#include <string>


class Texture : public Hazel::HazelTexture2D
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
	Texture();
	Texture(const char* fileLoc, bool flipVert = false);
	Texture(const char* fileLoc, bool flipVert, bool isSampler);
	Texture(const char* fileLoc, bool flipVert, bool isSampler, int filter);
	Texture(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter);
	Texture(const char* fileLoc, Specification spec); // constructor for fully customizable texture
	virtual ~Texture();

	virtual bool Load(bool flipVert = false);
	virtual void OpenGLCreate();
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

	void Unbind();
	void Clear();
	uint32_t CalculateMipMapCount(uint32_t width, uint32_t height); // used in Hazel::SceneRenderer
	uint32_t GetMipLevelCount();

	inline uint32_t GetID() const { return m_ID; };
	inline uint32_t GetWidth() const { return m_Spec.Width; };
	inline uint32_t GetHeight() const { return m_Spec.Height; };
	inline bool IsLoaded() const { return m_Buffer ? true : false; }; // used in Hazel::Mesh

	// Getters
	int GetRed(int x, int z);
	int GetGreen(int x, int z);
	int GetBlue(int x, int z);
	int GetAlpha(int x, int z);
	float GetFileSize(const char* filename);

	// Setters
	void SetPixel(int x, int z, glm::ivec4 pixel);
	void SetRed(int x, int z, int value);
	void SetGreen(int x, int z, int value);
	void SetBlue(int x, int z, int value);
	void SetAlpha(int x, int z, int value);

protected:
	uint32_t m_ID;
	const char* m_FileLocation;
	unsigned char* m_Buffer;
	int m_Level;
	Hazel::TextureFormat m_Format;

};
