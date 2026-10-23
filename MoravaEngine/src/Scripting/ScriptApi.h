#pragma once

#include <cstdint>


// The two function tables between C++ and C# (docs/engine/Game_Engine_Runtime_Guide.html, 4.3): plain C functions,
// blittable arguments only, strings as UTF-16 with a length. Same order as NativeApi and ManagedApi in
// ScriptCore/Interop.cs; a change here is a change there.

// What C# may call in C++: grows with each phase
struct ScriptNativeApi
{
	void (*Log)(int level, const char16_t* text, int length); // 0 info, 1 warning, 2 error
};

// What C++ calls in C#; Interop.Initialize fills it in. A script instance is a GCHandle of the C# object. The Invoke
// functions return 0, or 1 when the script threw (C# has logged it)
struct ScriptManagedApi
{
	int (*GetRuntimeVersion)(char16_t* buffer, int capacity);   // the length written, -1 when it doesn't fit
	int (*GetRuntimeDirectory)(char16_t* buffer, int capacity); // the folder of the base library
	int (*LoadGameAssembly)(const char16_t* path, int length);  // 0 when loaded
	int (*UnloadGameAssembly)();                                // 0 collected, 1 still alive, -1 refused (instances left)
	void* (*CreateInstance)(const char16_t* className, int length, uint64_t entity); // nullptr: no such class, or it threw
	void (*DestroyInstance)(void* instance);
	int (*InvokeCreate)(void* instance);
	int (*InvokeUpdate)(void* instance, float dt);
	int (*InvokeFixedUpdate)(void* instance, float dt);
};
