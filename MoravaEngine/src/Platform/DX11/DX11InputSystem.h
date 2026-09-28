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

	// Raw (unaccelerated, cursor-independent) mouse movement from WM_INPUT, accumulated between frames
	void RegisterRawMouseInput(HWND hwnd);
	void AddRawMouseDelta(long dx, long dy) { m_RawMouseDelta += glm::vec2((float)dx, (float)dy); }
	glm::vec2 ConsumeRawMouseDelta() { glm::vec2 delta = m_RawMouseDelta; m_RawMouseDelta = glm::vec2(0.0f); return delta; }

	static DX11InputSystem* Get();

private:
	glm::vec2 m_RawMouseDelta = glm::vec2(0.0f);

	std::list<DX11InputListener*> m_Listeners;

	unsigned char m_KeysState[256] = {};
	unsigned char m_OldKeysState[256] = {};

	glm::vec2 m_OldMousePos;
	bool m_FirstTime = true;

};
