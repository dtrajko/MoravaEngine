#include "DX11AppWindow.h"

#include "DX11InputSystem.h"


DX11AppWindow::DX11AppWindow()
{
	DX11InputSystem::Get()->AddListener(this);
}

DX11AppWindow::~DX11AppWindow()
{
}

void DX11AppWindow::Update()
{
	DX11InputSystem::Get()->Update();
}

void DX11AppWindow::OnKeyDown(int key)
{
}

void DX11AppWindow::OnKeyUp(int key)
{
}
