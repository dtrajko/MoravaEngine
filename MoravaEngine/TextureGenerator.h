#pragma once

#include "Texture.h"


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class TextureGenerator
{
public:
	static Texture* TextureFromColorMap(glm::vec4* colorMap, const char* fileLocation, int width, int height);
	static Texture* TextureFromHeightMap(float** noiseMap, const char* fileLocation, int width, int height);

};
