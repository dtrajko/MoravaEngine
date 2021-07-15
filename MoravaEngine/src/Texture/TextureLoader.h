#pragma once

#include "Texture/MoravaTexture.h"

#include <map>
#include <string>


class TextureLoader
{
public:
	TextureLoader();
	static TextureLoader* Get();
	Hazel::Ref<MoravaTexture> GetTexture(const char* fileLoc, bool flipVert, bool force);
	Hazel::Ref<MoravaTexture> GetTexture(const char* fileLoc, bool flipVert, GLenum filter, bool force);
	void Print();
	void Clean();
	~TextureLoader();

private:
	static TextureLoader* s_Instance;
	std::map<std::string, Hazel::Ref<MoravaTexture>> m_Textures;

};
