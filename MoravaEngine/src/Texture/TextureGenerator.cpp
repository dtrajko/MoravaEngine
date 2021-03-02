#include "Texture/TextureGenerator.h"

#include "../../NoiseSL.h"
#include "../../Math.h"


Texture* TextureGenerator::TextureFromHeightMap(float** noiseMap, const char* fileLocation, int width, int height)
{
	printf("TextureGenerator::TextureFromHeightMap\n");

	glm::vec4* colorMap = new glm::vec4[width * height];

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			colorMap[y * width + x] = glm::vec4(noiseMap[x][y], noiseMap[x][y], noiseMap[x][y], 1.0f);
		}
	}

	return TextureFromColorMap(colorMap, fileLocation, width, height);
}

Texture* TextureGenerator::TextureFromColorMap(glm::vec4* colorMap, const char* fileLocation, int width, int height)
{
	Texture* texture = new Texture(fileLocation, width, height, true, GL_LINEAR);

	constexpr int constValueIntMin = std::numeric_limits<int>::min();
	constexpr int constValueIntMax = std::numeric_limits<int>::max();

	int valueIntNormMin = constValueIntMax;
	int valueIntNormMax = constValueIntMin;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int normValueR = (int)Math::ConvertRangeFloat(colorMap[y * width + x].r, NoiseSL::s_NoiseHeightMin, NoiseSL::s_NoiseHeightMax, 0.0f, 255.0f);
			int normValueG = (int)Math::ConvertRangeFloat(colorMap[y * width + x].g, NoiseSL::s_NoiseHeightMin, NoiseSL::s_NoiseHeightMax, 0.0f, 255.0f);
			int normValueB = (int)Math::ConvertRangeFloat(colorMap[y * width + x].b, NoiseSL::s_NoiseHeightMin, NoiseSL::s_NoiseHeightMax, 0.0f, 255.0f);

			if (normValueR < valueIntNormMin) valueIntNormMin = normValueR;
			if (normValueR > valueIntNormMax) valueIntNormMax = normValueR;
			if (normValueG < valueIntNormMin) valueIntNormMin = normValueG;
			if (normValueG > valueIntNormMax) valueIntNormMax = normValueG;
			if (normValueB < valueIntNormMin) valueIntNormMin = normValueB;
			if (normValueB > valueIntNormMax) valueIntNormMax = normValueB;

			texture->SetPixel(x, y, glm::ivec4(normValueR, normValueG, normValueB, 255));
		}
	}

	printf("TextureGenerator::TextureFromColorMap Value Range [%.4ff-%.4ff]\n", NoiseSL::s_NoiseHeightMin, NoiseSL::s_NoiseHeightMax);
	printf("TextureGenerator::TextureFromColorMap Value Range Normalized [%i-%i]\n", valueIntNormMin, valueIntNormMax);

	texture->Save();

	return texture;
}
