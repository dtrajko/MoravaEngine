#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"


// TODO: Do we need this class? We can use glm::vec2 instead...
class DX11Point
{
public:
	DX11Point() : m_X(0), m_Y(0)
	{
	}

	DX11Point(int x, int y) : m_X(x), m_Y(y)
	{
	}

	// copy constructor
	DX11Point(const DX11Point& point) : m_X(point.m_X), m_Y(point.m_Y)
	{
	}

	~DX11Point()
	{
	}

public:
	int m_X = 0;
	int m_Y = 0;

};
