#include "TextureAtlas.h"


TextureAtlas::TextureAtlas(std::string fileName) : OpenGLMoravaTexture(fileName.c_str(), false)
{
	m_Rows = 1;
	m_Columns = 1;

	m_Spec.Texture_Wrap_S = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Wrap_T = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Wrap_R = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Min_Filter = GL_NEAREST;
	m_Spec.Texture_Mag_Filter = GL_NEAREST;

	printf("TextureAtlas constuctor: m_ID=%i Width=%i Height=%i\n", m_ID, m_Spec.Width, m_Spec.Height);
}

TextureAtlas::TextureAtlas(std::string fileName, unsigned int columns, unsigned int rows) : TextureAtlas(fileName)
{
	m_Columns = columns;
	m_Rows = rows;
}

TextureAtlas::TextureAtlas(std::string fileName, unsigned int columns, unsigned int rows, Specification spec) : TextureAtlas(fileName, columns, rows)
{
	m_Spec = spec;
}

void TextureAtlas::bind()
{
	m_Spec.MipLevel = 0;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ID);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, m_Spec.MipLevel, GL_TEXTURE_WIDTH, (GLint*)&m_Spec.Width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, m_Spec.MipLevel, GL_TEXTURE_HEIGHT, (GLint*)&m_Spec.Height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_R);
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
