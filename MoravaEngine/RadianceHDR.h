#pragma once

#include <GL/glew.h>

#include "Texture.h"


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
