#include "DX11InputSystem.h"

#include "Core/Application.h"
#include "Platform/Windows/WindowsWindow.h"

#include <Windows.h>


DX11InputSystem::DX11InputSystem()
{
}

DX11InputSystem::~DX11InputSystem()
{
}

void DX11InputSystem::AddListener(DX11InputListener* listener)
{
	m_MapListeners.insert(std::make_pair<DX11InputListener*, DX11InputListener*>
		(std::forward<DX11InputListener*>(listener), std::forward<DX11InputListener*>(listener)));
}

void DX11InputSystem::RemoveListener(DX11InputListener* listener)
{
	std::map<DX11InputListener*, DX11InputListener*>::iterator it = m_MapListeners.find(listener);

	if (it != m_MapListeners.end())
	{
		m_MapListeners.erase(it);
	}
}

void DX11InputSystem::SetCursorPosition(const glm::vec2& position)
{
	::SetCursorPos((int)position.x, (int)position.y);
}

void DX11InputSystem::Update()
{
	// MOUSE events
	POINT currentMousePos = {};
	::GetCursorPos(&currentMousePos);

	if (m_FirstTime)
	{
		m_OldMousePos = glm::vec2(currentMousePos.x, currentMousePos.y);
		m_FirstTime = false;
	}

	if (currentMousePos.x != m_OldMousePos.x || currentMousePos.y != m_OldMousePos.y)
	{
		// There is mouse move event
		std::map<DX11InputListener*, DX11InputListener*>::iterator it = m_MapListeners.begin();

		while (it != m_MapListeners.end())
		{
			glm::vec2 mousePosDelta = glm::vec2(currentMousePos.x - m_OldMousePos.x, currentMousePos.y - m_OldMousePos.y);
			glm::vec2 mousePosAbs = glm::vec2(currentMousePos.x, currentMousePos.y);
			it->second->OnMouseMove(mousePosDelta, mousePosAbs);
			++it;
		}
	}

	m_OldMousePos = glm::vec2(currentMousePos.x, currentMousePos.y);

	// KEYBOARD events
	if (::GetKeyboardState(m_KeysState))
	{
		for (unsigned int i = 0; i < 256; i++)
		{
			if (m_KeysState[i] & 0x80)
			{
				// Key is DOWN
				std::map<DX11InputListener*, DX11InputListener*>::iterator it = m_MapListeners.begin();

				while (it != m_MapListeners.end())
				{
					if (i == VK_LBUTTON)
					{
						if (m_KeysState[i] != m_OldKeysState[i])
						{
							it->second->OnLeftMouseDown(glm::vec2(currentMousePos.x, currentMousePos.y));
						}
					}

					if (i == VK_RBUTTON)
					{
						if (m_KeysState[i] != m_OldKeysState[i])
						{
							it->second->OnRightMouseDown(glm::vec2(currentMousePos.x, currentMousePos.y));
						}
					}

					it->second->OnKeyDown(i);
					++it;
				}
			}
			else
			{
				// Key is UP
				if (m_KeysState[i] != m_OldKeysState[i])
				{
					std::map<DX11InputListener*, DX11InputListener*>::iterator it = m_MapListeners.begin();

					while (it != m_MapListeners.end())
					{
						if (i == VK_LBUTTON)
						{
							it->second->OnLeftMouseUp(glm::vec2(currentMousePos.x, currentMousePos.y));
						}

						if (i == VK_RBUTTON)
						{
							it->second->OnRightMouseUp(glm::vec2(currentMousePos.x, currentMousePos.y));
						}

						it->second->OnKeyUp(i);
						++it;
					}
				}
			}
		}

		// store current keys state to old keys state buffer
		::memcpy(m_OldKeysState, m_KeysState, sizeof(unsigned char) * 256);
	}
}

DX11InputSystem* DX11InputSystem::Get()
{
	static DX11InputSystem system;
	return &system;
}

void DX11InputSystem::ShowCursor(bool show)
{
	::ShowCursor(show);
}

bool DX11InputSystem::IsMouseCursorAboveViewport()
{
	bool cursorAboveViewport = false;

	WindowsWindow* windowsWindow = (WindowsWindow*)Application::Get()->GetWindow();
	RECT windowRECT = windowsWindow->GetClientWindowRect();

	POINT currentMousePos = {};
	::GetCursorPos(&currentMousePos);

	if (currentMousePos.x >= windowRECT.left && currentMousePos.x <= windowRECT.right &&
		currentMousePos.y >= windowRECT.top && currentMousePos.y <= windowRECT.bottom)
	{
		cursorAboveViewport = true;
	}

	// Log::GetLogger()->info("DX11InputSystem::IsMouseCursorAboveViewport: '{0}'", cursorAboveViewport);

	return cursorAboveViewport;
}
