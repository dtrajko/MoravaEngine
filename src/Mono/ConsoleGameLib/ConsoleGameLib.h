#pragma once


#ifdef CGL_BUILD_DLL
// BUILD LIB
#define CGL_EXPORT __declspec(dllexport)
#else
// USE LIB
#define CGL_EXPORT __declspec(dllimport)
#endif

#define CONSOLE_STRW(str) str, wcslen(str)
#define CONSOLE_STRA(str) str, wcslen(str)
