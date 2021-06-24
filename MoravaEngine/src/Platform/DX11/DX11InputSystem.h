#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "DX11.h"
#include "DX11InputListener.h"
#include "DX11Point.h"

#include <map>


class DX11InputSystem
{
public:
	DX11InputSystem();
	~DX11InputSystem();

	void AddListener(DX11InputListener* listener);
	void RemoveListener(DX11InputListener* listener);

	void Update();

	static DX11InputSystem* Get();

private:
	std::map<DX11InputListener*, DX11InputListener*> m_MapListeners;

	unsigned char m_KeysState[256] = {};
	unsigned char m_OldKeysState[256] = {};

	DX11Point m_OldMousePos;
	bool m_FirstTime = true;

};
