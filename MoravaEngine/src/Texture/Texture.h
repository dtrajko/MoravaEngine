#pragma once

#include "../../Hazel/Core/Ref.h"

#include "Core/CommonValues.h"

#include <string>


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


class Texture : public Hazel::RefCounted
{
public:
	struct Specification
	{
		int InternalFormat;
		unsigned int Width;
		unsigned int Height;
		int Border;
		unsigned int Format;
		unsigned int Type;
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
	Texture(const char* fileLoc, unsigned int width, unsigned int height, bool isSampler, int filter);
	Texture(const char* fileLoc, Specification spec); // constructor for fully customizable texture
	virtual ~Texture();

	virtual bool Load(bool flipVert = false);
	virtual void OpenGLCreate();
	virtual void Save();
	virtual TextureFormat GetFormat() { return m_Format; };
	virtual void Bind(unsigned int textureSlot = 0);

	void Unbind();
	void Clear();
	unsigned int CalculateMipMapCount(unsigned int width, unsigned int height); // used in Hazel::SceneRenderer
	unsigned int GetMipLevelCount();

	inline unsigned int GetID() const { return m_ID; };
	inline unsigned int GetWidth() const { return m_Spec.Width; };
	inline unsigned int GetHeight() const { return m_Spec.Height; };
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
	unsigned int m_ID;
	const char* m_FileLocation;
	unsigned char* m_Buffer;
	int m_Level;
	TextureFormat m_Format;

};
