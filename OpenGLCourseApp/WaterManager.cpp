#include "WaterManager.h"


WaterManager::WaterManager()
{
}

WaterManager::WaterManager(int width, int height)
{
	m_Width = width;
	m_Height = height;

	reflectionFB = new Framebuffer();
	refractionFB = new Framebuffer();
}

WaterManager::~WaterManager()
{
	delete reflectionFB;
	delete refractionFB;
}
