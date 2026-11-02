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

	// MOUSE pure virtual callback functions
	virtual void OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs) = 0;

	virtual void OnLeftMouseDown(const glm::vec2& mousePos) = 0;
	virtual void OnRightMouseDown(const glm::vec2& mousePos) = 0;

	virtual void OnLeftMouseUp(const glm::vec2& mousePos) = 0;
	virtual void OnRightMouseUp(const glm::vec2& mousePos) = 0;

};
