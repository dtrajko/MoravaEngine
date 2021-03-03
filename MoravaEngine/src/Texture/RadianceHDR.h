#pragma once

#include "Texture/Texture.h"

#include <GL/glew.h>


class RadianceHDR : public Texture
{
public:
	RadianceHDR();
	RadianceHDR(const char* fileLoc);
	bool Load();
	~RadianceHDR();

private:
	float* m_Buffer;

};
