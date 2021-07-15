#ifndef _TEXTURE_ATLAS_H
#define _TEXTURE_ATLAS_H

#include "Platform/OpenGL/OpenGLMoravaTexture.h"

#include <GL/glew.h>

#include <string>


class TextureAtlas : public OpenGLMoravaTexture
{
private:
	unsigned int m_Rows;
	unsigned int m_Columns;

public:
	TextureAtlas(std::string fileName);
	TextureAtlas(std::string fileName, unsigned int columns, unsigned int rows);
	TextureAtlas(std::string fileName, unsigned int columns, unsigned int rows, Specification spec);
	virtual ~TextureAtlas();

	void bind();
	unsigned int getRows() const;
	unsigned int getColumns() const;
	unsigned int loadTexture(std::string fileName);

};

#endif
