#include "CsBind.h"


MonoString* CsBind::CS_RandomWord_getRandomWord()
{
	MonoString* ptrOutputString = nullptr;

	// Get randim word from CGL
	const char* randWord = CGL::getRandomWord();
	if (randWord)
	{
		ptrOutputString = mono_string_new(mono_domain_get(), randWord);
	}

	// Return result
	return ptrOutputString;
}

void CsBind::CS_GameConsole_putString(unsigned int x, unsigned int y, MonoString* str, bool wrap)
{
	CGL::GameConsole::putString(x, y, mono_string_to_utf8(str), wrap);
}
