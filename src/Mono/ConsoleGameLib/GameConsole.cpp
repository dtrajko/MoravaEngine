#include "GameConsole.h"
#include "GConsole.h"


namespace CGL
{
	void GameConsole::clearBuffer()
	{
		// Get and check console pointer
		auto consolePointer = CGL_internal::GConsole::get();
		if (consolePointer)
		{
			// Redirect call
			consolePointer->clearBuffer();
		}
	}

	void GameConsole::present()
	{
		// Get and check console pointer
		auto consolePointer = CGL_internal::GConsole::get();
		if (consolePointer)
		{
			// Redirect call
			consolePointer->present();
		}
	}

	void GameConsole::putChar(unsigned int x, unsigned int y, char c)
	{
		// Get and check console pointer
		auto consolePointer = CGL_internal::GConsole::get();
		if (consolePointer)
		{
			// Redirect call
			consolePointer->putChar(x, y, c);
		}
	}

	void GameConsole::putString(unsigned int x, unsigned int y, const char* string, bool wrap)
	{
		// Get and check console pointer
		auto consolePointer = CGL_internal::GConsole::get();
		if (consolePointer)
		{
			// Redirect call
			consolePointer->putString(x, y, string, wrap);
		}
	}
}
