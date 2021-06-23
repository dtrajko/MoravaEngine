#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11InputListener.h"


class DX11AppWindow : public DX11InputListener
{
public:
	DX11AppWindow();
	~DX11AppWindow();

	void Update();

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

};
