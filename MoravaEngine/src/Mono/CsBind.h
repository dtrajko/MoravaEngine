#pragma once

#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/debug-helpers.h>

// #include <ConsoleGameLib/RandomWord.h>
// #include <ConsoleGameLib/GameConsole.h>


// C# bindings
class CsBind
{
public:
	static MonoString* CS_RandomWord_getRandomWord();
	static void CS_GameConsole_putString(unsigned int x, unsigned int y, MonoString* str, bool wrap);

};
