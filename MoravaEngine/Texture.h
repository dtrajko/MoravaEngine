#pragma once

#include "CommonValues.h"

#include <string>


class Texture
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
	} m_Spec;

public:
	Texture();
	Texture(const char* fileLoc, bool flipVert = false);
	Texture(const char* fileLoc, bool flipVert, bool isSampler);
	Texture(const char* fileLoc, bool flipVert, bool isSampler, int filter);
	Texture(const char* fileLoc, unsigned int width, unsigned int height, bool isSampler, int filter);
	Texture(const char* fileLoc, Specification spec); // constructor for fully customizable texture
	~Texture();

	virtual bool Load(bool flipVert = false);
	virtual void OpenGLCreate();
	virtual void Save();
	inline unsigned int GetID() const { return m_TextureID; };
	virtual void Bind(unsigned int textureUnit = 0);
	void Unbind();
	void Clear();
	inline unsigned int GetWidth() const { return m_Spec.Width; };
	inline unsigned int GetHeight() const { return m_Spec.Height; };
	inline bool IsLoaded() const { return m_Buffer ? true : false; }; // used in Hazel::Mesh
	unsigned int CalculateMipMapCount(unsigned int width, unsigned int height); // used in Hazel::SceneRenderer
	unsigned int GetMipLevelCount();

	// Getters
	int GetRed(int x, int z);
	int GetGreen(int x, int z);
	int GetBlue(int x, int z);
	int GetAlpha(int x, int z);

	// Setters
	void SetPixel(int x, int z, glm::ivec4 pixel);
	void SetRed(int x, int z, int value);
	void SetGreen(int x, int z, int value);
	void SetBlue(int x, int z, int value);
	void SetAlpha(int x, int z, int value);

	float GetFileSize(const char* filename);

protected:
	unsigned int m_TextureID;
	const char* m_FileLocation;
	unsigned char* m_Buffer;
	int m_Level;

};
