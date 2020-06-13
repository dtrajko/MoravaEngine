#pragma once

#include "Texture.h"

#include <map>
#include <string>


class TextureLoader
{
public:
	TextureLoader();
	static TextureLoader* Get();
	Texture* GetTexture(const char* fileLoc, bool flipVert = false);
	Texture* GetTexture(const char* fileLoc, bool flipVert, GLenum filter);
	void Print();
	void Clean();
	~TextureLoader();

private:
	static TextureLoader* s_Instance;
	std::map<std::string, Texture*> m_Textures;

};
