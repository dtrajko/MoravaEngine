#pragma once

#include "Texture.h"


class TextureJoey : public Texture
{
public:
	TextureJoey();
	TextureJoey(const char* fileLoc, bool flipVert = false);
	virtual bool Load(bool flipVert = false) override;
	~TextureJoey();

};
