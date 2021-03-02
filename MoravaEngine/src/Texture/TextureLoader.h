#pragma once

#include "Texture/Texture.h"

#include <map>
#include <string>


class TextureLoader
{
public:
	TextureLoader();
	static TextureLoader* Get();
	Texture* GetTexture(const char* fileLoc, bool flipVert, bool force);
	Texture* GetTexture(const char* fileLoc, bool flipVert, GLenum filter, bool force);
	void Print();
	void Clean();
	~TextureLoader();

private:
	static TextureLoader* s_Instance;
	std::map<std::string, Texture*> m_Textures;

};
