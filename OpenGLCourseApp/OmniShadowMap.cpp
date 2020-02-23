#include "OmniShadowMap.h"

OmniShadowMap::OmniShadowMap() : ShadowMap() {}

bool OmniShadowMap::Init(GLuint width, GLuint height)
{
	shadowWidth = width;
	shadowHeight = height;

	glGenFramebuffers(1, &FBO);

	glGenTextures(1, &shadowMap);

	return false;
}

void OmniShadowMap::Write()
{
}

void OmniShadowMap::Read(unsigned int textureUnit)
{
}

OmniShadowMap::~OmniShadowMap()
{
}
