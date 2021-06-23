#include "DX11InputSystem.h"

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

void DX11InputSystem::Update()
{
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
