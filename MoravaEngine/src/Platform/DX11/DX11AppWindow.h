#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11InputListener.h"


// TODO: Do we need this class? We use DX11TestLayer as a class that implements the DX11InputListener interface
class DX11AppWindow : public DX11InputListener
{
public:
	DX11AppWindow();
	~DX11AppWindow();

	void Update();

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

	virtual void OnMouseMove(const DX11Point& deltaMousePos) override;

	virtual void OnLeftMouseDown(const DX11Point& deltaMousePos) override;
	virtual void OnRightMouseDown(const DX11Point& deltaMousePos) override;

	virtual void OnLeftMouseUp(const DX11Point& deltaMousePos) override;
	virtual void OnRightMouseUp(const DX11Point& deltaMousePos) override;

};
