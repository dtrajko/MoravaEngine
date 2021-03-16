#pragma once

#include "Hazel/Core/Ref.h"

#include <GL/glew.h>

#include <stdio.h>


class ShadowMap : public Hazel::RefCounted
{
public:
	ShadowMap();
	virtual bool Init(GLuint width, GLuint height);
	virtual void BindForWriting();
	virtual void ReadTexture(unsigned int textureUnit);
	virtual void Unbind(unsigned int width, unsigned int height);
	GLuint GetShadowWidth() { return m_ShadowWidth; };
	GLuint GetShadowHeight() { return m_ShadowHeight; };
	inline const unsigned int GetFBO() const { return m_FBO; };
	inline const unsigned int GetTextureID() const { return m_TextureID; };
	virtual ~ShadowMap();

protected:
	GLuint m_FBO;
	GLuint m_TextureID;
	GLuint m_ShadowWidth;
	GLuint m_ShadowHeight;

};
