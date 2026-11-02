#pragma once

#include <random>


class Random
{
public:
	static void Init();
	static int Int();
	static float Float();
	static uint32_t GetMax();

private:
	static std::mt19937 s_RandomEngine;
	static std::uniform_int_distribution<std::mt19937::result_type> s_Distribution;

};
