#include "TextureAtlas.h"


TextureAtlas::TextureAtlas(std::string fileName) : Texture(fileName.c_str(), false)
{
	m_Rows = 1;
	m_Columns = 1;

	printf("TextureAtlas constuctor: m_ID=%i Width=%i Height=%i\n", m_TextureID, m_Width, m_Height);
}

TextureAtlas::TextureAtlas(std::string fileName, unsigned int columns, unsigned int rows) : TextureAtlas(fileName)
{
	m_Columns = columns;
	m_Rows = rows;
}

void TextureAtlas::bind()
{
	int mipLevel = 0;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_WIDTH, (GLint*)&m_Width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_HEIGHT, (GLint*)&m_Height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

unsigned int TextureAtlas::getRows() const
{
	return m_Rows;
}

unsigned int TextureAtlas::getColumns() const
{
	return m_Columns;
}

unsigned int TextureAtlas::loadTexture(std::string fileName)
{
	// TODO
	return 0;
}

TextureAtlas::~TextureAtlas()
{

}
