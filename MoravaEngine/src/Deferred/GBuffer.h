#pragma once

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))

#include <GL/glew.h>


class GBuffer
{

public:
	enum GBUFFER_TEXTURE_TYPE {
		GBUFFER_TEXTURE_TYPE_POSITION,
		GBUFFER_TEXTURE_TYPE_DIFFUSE,
		GBUFFER_TEXTURE_TYPE_NORMAL,
		GBUFFER_TEXTURE_TYPE_TEXCOORD,
		GBUFFER_NUM_TEXTURES
	};

	GBuffer();
	~GBuffer();

	bool Init(unsigned int WindowWidth, unsigned int WindowHeight);
	void BindForWriting();
	void BindForReading();
	void SetReadBuffer(GBUFFER_TEXTURE_TYPE TextureType);

private:
	GLuint m_fbo;
	GLuint m_textures[GBUFFER_NUM_TEXTURES];
	GLuint m_depthTexture;

};
