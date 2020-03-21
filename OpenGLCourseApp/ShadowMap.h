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
	GLuint GetShadowWidth() { return shadowWidth; };
	GLuint GetShadowHeight() { return shadowHeight; };
	~ShadowMap();

protected:

	GLuint FBO;
	GLuint shadowMap;
	GLuint shadowWidth;
	GLuint shadowHeight;


};
