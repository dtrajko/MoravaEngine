#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"


class DX11InputListener
{
public:
	DX11InputListener()
	{
	}

	~DX11InputListener()
	{
	}

	// KEYBOARD pure virtual callback functions
	virtual void OnKeyDown(int key) = 0;
	virtual void OnKeyUp(int key) = 0;

};
