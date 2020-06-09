#pragma once

#include <stdio.h>
#include <GL/glew.h>


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
	inline const unsigned int GetID() const { return m_ID; };
	~ShadowMap();

protected:

	GLuint FBO;
	GLuint m_ID;
	GLuint shadowWidth;
	GLuint shadowHeight;


};
