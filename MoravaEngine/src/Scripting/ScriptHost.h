#pragma once

#include "ScriptApi.h"
#include "ScriptField.h"

#include <cstdint>
#include <filesystem>
#include <string>


// Strings between C# (UTF-16) and the engine (UTF-8)
std::string ScriptUtf16ToUtf8(const char16_t* text, int length);
std::u16string ScriptUtf8ToUtf16(const std::string& text);

enum class ScriptUnloadResult
{
	Unloaded,   // the game's context is collected: its file can be loaded again, rebuilt or not
	StillAlive, // something still holds the game (a leak): a reload would run the old code next to the new
	Refused,    // script instances still exist
};

/**
 * C# on .NET in the engine (docs/engine/Game_Engine_Runtime_Guide.html, 4.2-4.3). Initialize starts the runtime from the
 * private .NET folder next to the executable (dotnet/: as in a shipped game, never an installed .NET), loads
 * Morava.ScriptCore.dll from the same folder and exchanges the two function tables of ScriptApi.h with it.
 *
 * A game's scripts are classes deriving from Morava.Entity in the game's assembly, loaded from bytes into a collectible
 * context (the file stays free for the next build) and unloaded again for a reload. .NET can't be stopped and started
 * again in a process: Shutdown unloads the game, the runtime ends with the process.
 *
 * Without the runtime (a build with MORAVA_SCRIPTING=OFF, or a missing dotnet/ folder: logged once) IsAvailable is false
 * and every call does nothing.
 */
class ScriptHost
{
public:
	// Once per process, at startup (Application::OnInit); false when scripting isn't available
	static bool Initialize();
	static void Shutdown();

	static bool IsAvailable();
	static const std::string& GetRuntimeVersion();             // "10.0.12"
	static const std::filesystem::path& GetRuntimeDirectory(); // the base library's folder in dotnet/

	static bool LoadGameAssembly(const std::filesystem::path& path);
	static ScriptUnloadResult UnloadGameAssembly();
	static bool IsGameAssemblyLoaded();
	static const std::filesystem::path& GetGameAssemblyPath(); // the one loaded (empty: none)

	// The loaded game's script classes with their public fields (4.5), sorted by name; empty without a game
	static const std::vector<ScriptClass>& GetClasses();
	static const ScriptClass* FindClass(const std::string& name);

	// An instance of a game's script class for an entity, nullptr when there is none (logged). OnCreate is called by
	// InvokeCreate, so the script's fields can be set in between. The Invoke functions return false when the script threw.
	static void* CreateInstance(const std::string& className, uint64_t entity);
	static void DestroyInstance(void* instance);
	static bool InvokeCreate(void* instance);
	static bool InvokeUpdate(void* instance, float dt);
	static bool InvokeFixedUpdate(void* instance, float dt);
	// other: the entity it touched or that entered or left (a trigger); point and normal for CollisionEnter
	static bool InvokeCollision(void* instance, ScriptCollisionKind kind, uint64_t other, const glm::vec3& point, const glm::vec3& normal);

	// A public field of an instance (field: of its class, GetClasses). The value must be of the field's type (an Enum by
	// its name); false when it isn't, or C# refused it (logged)
	static bool SetField(void* instance, const ScriptFieldInfo& field, const ScriptFieldValue& value);
	static bool GetField(void* instance, const ScriptFieldInfo& field, ScriptFieldValue& value);

	// The functions C# calls, which it reads through a pointer: ScriptBindings sets the game's into it
	static ScriptNativeApi& GetNativeApi();
};
