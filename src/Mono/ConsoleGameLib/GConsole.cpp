#include "GConsole.h"

// Static instance
CGL_internal::GConsole* CGL_internal::GConsole::s_ptrInstance = nullptr;

bool CGL_internal::GConsole::init()
{
	// Create and check console
	s_ptrInstance = new GConsole();
	if (s_ptrInstance)
	{
		// Check console handle valid
		if (s_ptrInstance->m_hConsole != INVALID_HANDLE_VALUE)
		{
			return true;
		}
		else
		{
			// Delete and unset console
			delete s_ptrInstance;
			s_ptrInstance = nullptr;
		}
	}

	// Fallback
	return false;
}

void CGL_internal::GConsole::destroy()
{
	// Destroy if present and unset
	if (s_ptrInstance)
	{
		delete s_ptrInstance;
		s_ptrInstance = nullptr;
	}
}

CGL_internal::GConsole* CGL_internal::GConsole::get()
{
	return s_ptrInstance;
}

CGL_internal::GConsole::GConsole()
{
	// Alloc and check console
	if (AllocConsole())
	{
		// Get output handle
		HANDLE hTempConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hTempConsole != INVALID_HANDLE_VALUE)
		{
			// Retrieve console mode
			DWORD mode;
			if (GetConsoleMode(hTempConsole, &mode))
			{
				// Add virtual terminal mode
				mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				// Set new mode
				if (SetConsoleMode(hTempConsole, mode))
				{
					// Store handle in class
					m_hConsole = hTempConsole;
				}
			}
		}
	}
}

CGL_internal::GConsole::~GConsole()
{
	// Only if valid
	if (m_hConsole != INVALID_HANDLE_VALUE)
	{
		// Free console again
		FreeConsole();
	}
}

void CGL_internal::GConsole::clearBuffer()
{
	// For each line
	for (unsigned int y = 0; y < CGL_internal_CONSOLE_HEIGHT; y++)
	{
		// Get line pointer
		wchar_t* lineBegin = &m_consoleBuffer[(CGL_internal_CONSOLE_WIDTH + 1) * y];

		// Memset spaces anbd set newline
		memset(lineBegin, ' ', CGL_internal_CONSOLE_WIDTH);
		lineBegin[CGL_internal_CONSOLE_WIDTH] = L'\n';
	}

	// Set terminator char
	m_consoleBuffer[(CGL_internal_CONSOLE_WIDTH + 1) * CGL_internal_CONSOLE_HEIGHT] = L'\0';
}

void CGL_internal::GConsole::present()
{
	// First clear old content
	WriteConsoleW(m_hConsole, CONSOLE_STRW(L"\x1b[2J"), NULL, NULL);
	system("cls");
	// Present new content
	WriteConsoleA(m_hConsole, CONSOLE_STRA(m_consoleBuffer), NULL, NULL);
}

void CGL_internal::GConsole::putChar(unsigned int x, unsigned int y, char c)
{
	// Check bounds
	if (x < CGL_internal_CONSOLE_WIDTH && y < CGL_internal_CONSOLE_HEIGHT)
	{
		// Put char
		m_consoleBuffer[(CGL_internal_CONSOLE_WIDTH + 1) * y + x] = c;
	}
}

void CGL_internal::GConsole::putString(unsigned int x, unsigned int y, const char* string, bool wrap)
{
	// While has chars
	while (*string)
	{
		// Put char to console
		putChar(x, y, *string);

		// Increment x
		x++;

		// Check if end of line
		if (x >= CGL_internal_CONSOLE_WIDTH)
		{
			if (wrap)
			{
				// Wrap to next line
				y++;
				x = 0;
			}
			else
			{
				// End of write reached
				return;
			}
		}

		// Increment string pointer
		string++;
	}
}
