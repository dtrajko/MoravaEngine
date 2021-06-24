#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11Point.h"


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

	// MOUSE pure virtual callback functions
	virtual void OnMouseMove(const DX11Point& deltaMousePos) = 0;

	virtual void OnLeftMouseDown(const DX11Point& deltaMousePos) = 0;
	virtual void OnRightMouseDown(const DX11Point& deltaMousePos) = 0;

	virtual void OnLeftMouseUp(const DX11Point& deltaMousePos) = 0;
	virtual void OnRightMouseUp(const DX11Point& deltaMousePos) = 0;

};
