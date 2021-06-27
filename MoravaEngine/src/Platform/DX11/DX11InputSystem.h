#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11InputListener.h"

#include <list>


class DX11InputSystem
{
public:
	DX11InputSystem();
	~DX11InputSystem();

	void Update();

	void AddListener(DX11InputListener* listener);
	void RemoveListener(DX11InputListener* listener);

	void SetCursorPosition(const glm::vec2& position);
	void ShowCursor(bool show);

	bool IsMouseCursorAboveViewport();

	static DX11InputSystem* Get();

private:
	std::list<DX11InputListener*> m_Listeners;

	unsigned char m_KeysState[256] = {};
	unsigned char m_OldKeysState[256] = {};

	glm::vec2 m_OldMousePos;
	bool m_FirstTime = true;

};
