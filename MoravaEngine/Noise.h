#pragma once

#include <vector>


class Noise
{
public:
	Noise();
	std::vector<float> GenerateNoiseMap(int mapWidth, int mapHeight);
	~Noise();

private:

};
