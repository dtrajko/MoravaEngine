#pragma once

#include "Mono/ConsoleGameLib/ConsoleGameLib.h"


namespace CGL
{
	// Export console class
	class GameConsole
	{
	public:
		// Clear console buffer
		static void clearBuffer();

		// Present console buffer to user
		static void present();

		// Put char on offscreen console buffer
		static void putChar(unsigned int x, unsigned int y, char c);

		// Put string on offscreen console buffer (wrapping can be enabled or disabled)
		static void putString(unsigned int x, unsigned int y, const char* string, bool wrap = true);

	};

} // end namespace CGL
