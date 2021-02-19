#include "TextureCubemap.h"
#include "CommonValues.h"

#include <GL/glew.h>


TextureCubemap::TextureCubemap()
	: Texture()
{
	m_Spec.InternalFormat = GL_RGB16F;
	m_Spec.Border = 0;
	m_Spec.Format = GL_RGB;
	m_Spec.Type = GL_FLOAT;
	m_Spec.Texture_Wrap_S = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Wrap_T = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Wrap_R = GL_CLAMP_TO_EDGE;
	m_Spec.Texture_Min_Filter = GL_LINEAR;
	m_Spec.Texture_Mag_Filter = GL_LINEAR;

	m_Level = 0;
}

TextureCubemap::TextureCubemap(unsigned int width, unsigned int height)
	: TextureCubemap()
{
	m_Spec.Width = width;
	m_Spec.Height = height;

	OpenGLCreate();
}

TextureCubemap::TextureCubemap(Texture::Specification spec)
{
	m_Spec.InternalFormat     = spec.InternalFormat;
	m_Spec.Width              = spec.Width;
	m_Spec.Height             = spec.Height;
	m_Spec.Border             = spec.Border;
	m_Spec.Format             = spec.Format;
	m_Spec.Type               = spec.Type;
	m_Spec.Texture_Wrap_S     = spec.Texture_Wrap_S;
	m_Spec.Texture_Wrap_T     = spec.Texture_Wrap_T;
	m_Spec.Texture_Wrap_R     = spec.Texture_Wrap_R;
	m_Spec.Texture_Min_Filter = spec.Texture_Min_Filter;
	m_Spec.Texture_Mag_Filter = spec.Texture_Mag_Filter;
	m_Spec.MipLevel           = spec.MipLevel;
	m_Spec.FlipVertically     = spec.FlipVertically;
	m_Spec.BitDepth           = spec.BitDepth;
	m_Spec.IsSampler          = spec.IsSampler;
	m_Spec.Samples            = spec.Samples;
	m_Spec.IsMultisample      = spec.IsMultisample;
	m_Spec.IsSRGB             = spec.IsSRGB;

	OpenGLCreate();
}

void TextureCubemap::OpenGLCreate()
{
	glGenTextures(1, &m_ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

	for (unsigned int i = 0; i < 6; ++i)
	{
		// note that we store each face with 16-bit floating point values
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_Level, m_Spec.InternalFormat, m_Spec.Width, m_Spec.Height, m_Spec.Border, m_Spec.Format, m_Spec.Type, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, m_Spec.Texture_Wrap_R);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

	printf("TextureCubemap GL_TEXTURE_CUBE_MAP m_TextureID=%d\n", m_ID);
}

void TextureCubemap::Bind(unsigned int textureSlot)
{
	glActiveTexture(GL_TEXTURE0 + textureSlot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
}

TextureCubemap::~TextureCubemap()
{
}
