#pragma once

#include "Texture/Texture.h"

#include <map>
#include <string>


class TextureLoader
{
public:
	TextureLoader();
	static TextureLoader* Get();
	Hazel::Ref<Texture> GetTexture(const char* fileLoc, bool flipVert, bool force);
	Hazel::Ref<Texture> GetTexture(const char* fileLoc, bool flipVert, GLenum filter, bool force);
	void Print();
	void Clean();
	~TextureLoader();

private:
	static TextureLoader* s_Instance;
	std::map<std::string, Hazel::Ref<Texture>> m_Textures;

};
