// Phase 4.1 spike: hosts .NET from a private folder next to the executable (dotnet/), as a shipped game will, and checks
// each step: find hostfxr there, start the runtime, call C# and be called back, survive a C# exception, load a game
// assembly into a collectible context, overwrite its file while loaded, unload it, check it was collected, load the
// new build. Exit code 0 when every check passed.

#include <nethost.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #define STR(s) L##s
#else
    #include <dlfcn.h>
    #define STR(s) s
    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
#endif

namespace fs = std::filesystem;

struct Vec3 { float x, y, z; };

// Same layout as NativeApi and ManagedApi in Managed/SpikeCore/Interop.cs
struct NativeApi
{
    void (*Log)(int level, const char16_t* text, int length);
    void (*GetTranslation)(uint64_t entity, Vec3* out);
    void (*SetTranslation)(uint64_t entity, const Vec3* value);
};

struct ManagedApi
{
    int (*GetRuntimeDirectory)(char16_t* buffer, int capacity);
    int (*LoadGame)(const char16_t* path, int length);
    int (*UnloadGame)();
    void* (*CreateInstance)(const char16_t* className, int length, uint64_t entity);
    void (*InvokeUpdate)(void* instance, float dt);
    void (*DestroyInstance)(void* instance);
};

static std::vector<Vec3> s_Translations(4, Vec3{ 0, 0, 0 });
static int s_ErrorCount = 0;
static bool s_QuietErrors = false;
static int s_Failures = 0;

static std::string ToUtf8(const char16_t* text, int length)
{
    std::string out;
    for (int i = 0; i < length; i++)
    {
        uint32_t c = text[i];
        if (c >= 0xD800 && c < 0xDC00 && i + 1 < length)
            c = 0x10000 + ((c - 0xD800) << 10) + (text[++i] - 0xDC00);
        if (c < 0x80) out += char(c);
        else if (c < 0x800) { out += char(0xC0 | (c >> 6)); out += char(0x80 | (c & 0x3F)); }
        else if (c < 0x10000) { out += char(0xE0 | (c >> 12)); out += char(0x80 | ((c >> 6) & 0x3F)); out += char(0x80 | (c & 0x3F)); }
        else { out += char(0xF0 | (c >> 18)); out += char(0x80 | ((c >> 12) & 0x3F)); out += char(0x80 | ((c >> 6) & 0x3F)); out += char(0x80 | (c & 0x3F)); }
    }
    return out;
}

static void Check(bool condition, const std::string& what)
{
    std::printf("%s  %s\n", condition ? "PASS" : "FAIL", what.c_str());
    if (!condition) s_Failures++;
}

// The functions C# calls
static void NativeLog(int level, const char16_t* text, int length)
{
    if (level >= 2) s_ErrorCount++;
    if (level >= 2 && s_QuietErrors) return;
    std::printf("      [C#%s] %s\n", level >= 2 ? " error" : "", ToUtf8(text, length).c_str());
}

static void NativeGetTranslation(uint64_t entity, Vec3* out) { *out = s_Translations.at(entity); }
static void NativeSetTranslation(uint64_t entity, const Vec3* value) { s_Translations.at(entity) = *value; }

static fs::path ExecutablePath()
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return buffer;
#elif defined(__APPLE__)
    char buffer[4096];
    uint32_t size = sizeof(buffer);
    _NSGetExecutablePath(buffer, &size);
    return fs::canonical(buffer);
#else
    return fs::read_symlink("/proc/self/exe");
#endif
}

static void* LoadLibraryAt(const fs::path& path)
{
#ifdef _WIN32
    return LoadLibraryW(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

template<typename T>
static T Export(void* library, const char* name)
{
#ifdef _WIN32
    return reinterpret_cast<T>(GetProcAddress(static_cast<HMODULE>(library), name));
#else
    return reinterpret_cast<T>(dlsym(library, name));
#endif
}

static bool IsInside(const fs::path& path, const fs::path& folder)
{
    fs::path relative = fs::weakly_canonical(path).lexically_relative(fs::weakly_canonical(folder));
    return !relative.empty() && *relative.begin() != "..";
}

static void HostfxrError(const char_t* message)
{
#ifdef _WIN32
    std::fwprintf(stderr, L"      [hostfxr] %ls\n", message);
#else
    std::fprintf(stderr, "      [hostfxr] %s\n", message);
#endif
}

static void UpdateFrames(const ManagedApi& managed, const std::vector<void*>& instances, int frames)
{
    for (int frame = 0; frame < frames; frame++)
        for (void* instance : instances)
            managed.InvokeUpdate(instance, 1.0f / 60.0f);
}

static void* Create(const ManagedApi& managed, const std::u16string& className, uint64_t entity)
{
    return managed.CreateInstance(className.c_str(), int(className.size()), entity);
}

int main()
{
    const fs::path exe = ExecutablePath();
    const fs::path folder = exe.parent_path();
    const fs::path dotnetRoot = folder / "dotnet";
    const fs::path coreDll = folder / "SpikeCore.dll";
    std::printf("Executable: %s\n", exe.string().c_str());

    // 1. hostfxr from the private folder only
    get_hostfxr_parameters hostfxrParams { sizeof(hostfxrParams), nullptr, dotnetRoot.c_str() };
    char_t hostfxrBuffer[1024];
    size_t hostfxrSize = sizeof(hostfxrBuffer) / sizeof(char_t);
    int result = get_hostfxr_path(hostfxrBuffer, &hostfxrSize, &hostfxrParams);
    const fs::path hostfxrPath = result == 0 ? fs::path(hostfxrBuffer) : fs::path();
    Check(result == 0 && IsInside(hostfxrPath, dotnetRoot), "hostfxr found in the private .NET folder: " + hostfxrPath.string());
    if (s_Failures) return 1;

    void* hostfxr = LoadLibraryAt(hostfxrPath);
    auto initialize = Export<hostfxr_initialize_for_runtime_config_fn>(hostfxr, "hostfxr_initialize_for_runtime_config");
    auto getDelegate = Export<hostfxr_get_runtime_delegate_fn>(hostfxr, "hostfxr_get_runtime_delegate");
    auto closeContext = Export<hostfxr_close_fn>(hostfxr, "hostfxr_close");
    auto setErrorWriter = Export<hostfxr_set_error_writer_fn>(hostfxr, "hostfxr_set_error_writer");
    Check(hostfxr && initialize && getDelegate && closeContext && setErrorWriter, "hostfxr loaded, exports found");
    if (s_Failures) return 1;
    setErrorWriter(HostfxrError);

    // 2. The runtime, from SpikeCore's runtimeconfig.json, with the private folder as dotnet_root
    fs::path config = coreDll;
    config.replace_extension(".runtimeconfig.json");
    hostfxr_initialize_parameters initParams { sizeof(initParams), exe.c_str(), dotnetRoot.c_str() };
    hostfxr_handle context = nullptr;
    result = initialize(config.c_str(), &initParams, &context);
    Check(result == 0 && context, "runtime initialized from " + config.filename().string());
    if (s_Failures) return 1;

    load_assembly_fn loadAssembly = nullptr;
    get_function_pointer_fn getFunctionPointer = nullptr;
    getDelegate(context, hdt_load_assembly, reinterpret_cast<void**>(&loadAssembly));
    getDelegate(context, hdt_get_function_pointer, reinterpret_cast<void**>(&getFunctionPointer));
    closeContext(context);
    Check(loadAssembly && getFunctionPointer, "runtime delegates: load_assembly, get_function_pointer");
    if (s_Failures) return 1;

    // 3. SpikeCore into the default context; the two function tables exchanged
    result = loadAssembly(coreDll.c_str(), nullptr, nullptr);
    using InitializeFn = int (CORECLR_DELEGATE_CALLTYPE*)(const NativeApi*, ManagedApi*);
    InitializeFn initializeCore = nullptr;
    if (result == 0)
        result = getFunctionPointer(STR("Morava.Spike.Interop, SpikeCore"), STR("Initialize"), UNMANAGEDCALLERSONLY_METHOD,
                                    nullptr, nullptr, reinterpret_cast<void**>(&initializeCore));
    static const NativeApi native { NativeLog, NativeGetTranslation, NativeSetTranslation };
    ManagedApi managed {};
    Check(result == 0 && initializeCore && initializeCore(&native, &managed) == 0 && managed.DestroyInstance,
          "SpikeCore loaded, Interop.Initialize called, ManagedApi filled in");
    if (s_Failures) return 1;

    char16_t runtimeBuffer[1024];
    int runtimeLength = managed.GetRuntimeDirectory(runtimeBuffer, 1024);
    const fs::path runtimeDirectory = fs::u8path(ToUtf8(runtimeBuffer, runtimeLength > 0 ? runtimeLength : 0));
    Check(IsInside(runtimeDirectory, dotnetRoot), "base library from the private .NET folder: " + runtimeDirectory.string());

    // 4. The game, version 1, from game/ (loaded from bytes); an exception in a script doesn't reach C++
    const fs::path gameDll = folder / "game" / "SpikeGame.dll";
    auto installGame = [&](const char* version) {
        std::error_code error;
        fs::create_directories(gameDll.parent_path(), error);
        for (const char* extension : { ".dll", ".pdb" })
        {
            fs::path from = folder / (std::string("game_") + version) / (std::string("SpikeGame") + extension);
            fs::path to = gameDll;
            fs::copy_file(from, to.replace_extension(extension), fs::copy_options::overwrite_existing, error);
            if (error) return false;
        }
        return true;
    };
    const std::u16string gamePath = gameDll.u16string();
    Check(installGame("v1") && managed.LoadGame(gamePath.c_str(), int(gamePath.size())) == 0, "game v1 loaded into a collectible context");

    std::vector<void*> instances { Create(managed, u"Mover", 0), Create(managed, u"Thrower", 1) };
    Check(instances[0] && instances[1], "instances created: Mover on entity 0, Thrower on entity 1");
    if (s_Failures) return 1;
    s_ErrorCount = 0;
    UpdateFrames(managed, instances, 1);
    s_QuietErrors = true;   // the same error 59 more times
    UpdateFrames(managed, instances, 59);
    s_QuietErrors = false;
    Check(std::fabs(s_Translations[0].x - 1.0f) < 1e-3f, "Mover v1 moved entity 0 to x = " + std::to_string(s_Translations[0].x) + " (expected 1)");
    Check(s_ErrorCount == 60, "Thrower's exception caught at the boundary 60 times, the process lives on");

    // 5. Hot reload: a new build overwrites the loaded file, the old game is unloaded and collected, the new one loads
    Check(installGame("v2"), "game v2 copied over the loaded game's files (not locked)");
    for (void* instance : instances)
        managed.DestroyInstance(instance);
    Check(managed.UnloadGame() == 0, "game v1 unloaded, its context collected");
    Check(managed.LoadGame(gamePath.c_str(), int(gamePath.size())) == 0, "game v2 loaded");
    instances = { Create(managed, u"Mover", 0) };
    UpdateFrames(managed, instances, 60);
    Check(std::fabs(s_Translations[0].x - 3.0f) < 1e-3f, "Mover v2 moved entity 0 on to x = " + std::to_string(s_Translations[0].x) + " (expected 1 + 2)");
    managed.DestroyInstance(instances[0]);
    Check(managed.UnloadGame() == 0, "game v2 unloaded, its context collected");

    // 6. A leak is seen: a game object left in an engine list keeps the context alive, and unloading reports it
    Check(managed.LoadGame(gamePath.c_str(), int(gamePath.size())) == 0, "game v2 loaded again");
    managed.DestroyInstance(Create(managed, u"Leaker", 0));
    s_QuietErrors = true;
    Check(managed.UnloadGame() == 1, "a game kept alive by an engine list is reported, not taken as unloaded");
    s_QuietErrors = false;

    std::printf(s_Failures ? "\n%d check(s) failed\n" : "\nAll checks passed\n", s_Failures);
    return s_Failures ? 1 : 0;
}
