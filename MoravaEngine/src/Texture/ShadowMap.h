#pragma once

#include <GL/glew.h>

#include <stdio.h>


class ShadowMap
{
public:
	ShadowMap();
	virtual bool Init(GLuint width, GLuint height);
	virtual void Write();
	virtual void Read(unsigned int textureUnit);
	virtual void Unbind(unsigned int width, unsigned int height);
	GLuint GetShadowWidth() { return shadowWidth; };
	GLuint GetShadowHeight() { return shadowHeight; };
	inline const unsigned int GetFBO() const { return m_FBO; };
	inline const unsigned int GetTextureID() const { return m_TextureID; };
	virtual ~ShadowMap();

protected:
	GLuint m_FBO;
	GLuint m_TextureID;
	GLuint shadowWidth;
	GLuint shadowHeight;

};
