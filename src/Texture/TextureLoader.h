#pragma once

#include "Texture/MoravaTexture.h"

#include <map>
#include <string>


class TextureLoader
{
public:
	TextureLoader();
	static TextureLoader* Get();
	H2M::RefH2M<MoravaTexture> GetTexture(const char* fileLoc, bool flipVert, bool force);
	H2M::RefH2M<MoravaTexture> GetTexture(const char* fileLoc, bool flipVert, GLenum filter, bool force);
	void Print();
	void Clean();
	~TextureLoader();

private:
	static TextureLoader* s_Instance;
	std::map<std::string, H2M::RefH2M<MoravaTexture>> m_Textures;

};
