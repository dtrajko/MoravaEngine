#include "ScriptHost.h"

#include "ScriptApi.h"

#include "Core/Log.h"
#include "Core/Process.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iterator>
#include <system_error>

#if defined(MORAVA_SCRIPTING)
	// From the .NET SDK's host pack (Microsoft.NETCore.App.Host.<rid>); char_t is wchar_t on Windows, char elsewhere
	#include <nethost.h>
	#include <hostfxr.h>
	#include <coreclr_delegates.h>

	#if defined(_WIN32)
		#include <windows.h>
		#define SCRIPT_HOST_STR(s) L##s
	#else
		#include <dlfcn.h>
		#define SCRIPT_HOST_STR(s) s
	#endif
#endif


namespace fs = std::filesystem;

static bool s_Available = false;
static bool s_Initialized = false;
static bool s_GameAssemblyLoaded = false;
static fs::path s_GameAssemblyPath;
static fs::file_time_type s_GameAssemblyWriteTime;
static uint32_t s_LeakedAssemblyCount = 0;
static std::vector<ScriptClass> s_Classes;
static std::string s_RuntimeVersion;
static fs::path s_RuntimeDirectory;
static ScriptManagedApi s_Managed {};

std::string ScriptUtf16ToUtf8(const char16_t* text, int length)
{
	std::string out;
	out.reserve(length);
	for (int i = 0; i < length; i++)
	{
		uint32_t c = text[i];
		if (c >= 0xD800 && c < 0xDC00 && i + 1 < length)
		{
			c = 0x10000 + ((c - 0xD800) << 10) + (text[++i] - 0xDC00u);
		}
		if (c < 0x80)
		{
			out += char(c);
		}
		else if (c < 0x800)
		{
			out += char(0xC0 | (c >> 6));
			out += char(0x80 | (c & 0x3F));
		}
		else if (c < 0x10000)
		{
			out += char(0xE0 | (c >> 12));
			out += char(0x80 | ((c >> 6) & 0x3F));
			out += char(0x80 | (c & 0x3F));
		}
		else
		{
			out += char(0xF0 | (c >> 18));
			out += char(0x80 | ((c >> 12) & 0x3F));
			out += char(0x80 | ((c >> 6) & 0x3F));
			out += char(0x80 | (c & 0x3F));
		}
	}
	return out;
}

std::u16string ScriptUtf8ToUtf16(const std::string& text)
{
	std::u16string out;
	out.reserve(text.size());
	for (size_t i = 0; i < text.size();)
	{
		const unsigned char lead = static_cast<unsigned char>(text[i]);
		const int extra = lead < 0x80 ? 0 : lead < 0xE0 ? 1 : lead < 0xF0 ? 2 : 3;
		uint32_t c = extra == 0 ? lead : extra == 1 ? (lead & 0x1F) : extra == 2 ? (lead & 0x0F) : (lead & 0x07);
		for (int k = 1; k <= extra && i + k < text.size(); k++)
		{
			c = (c << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
		}
		i += extra + 1;
		if (c >= 0x10000)
		{
			c -= 0x10000;
			out += char16_t(0xD800 + (c >> 10));
			out += char16_t(0xDC00 + (c & 0x3FF));
		}
		else
		{
			out += char16_t(c);
		}
	}
	return out;
}

// What C# calls (ScriptNativeApi)
static void NativeLog(int level, const char16_t* text, int length)
{
	const std::string message = ScriptUtf16ToUtf8(text, length);
	switch (level)
	{
	case 0:  Log::GetLogger()->info("C#: {0}", message); break;
	case 1:  Log::GetLogger()->warn("C#: {0}", message); break;
	default: Log::GetLogger()->error("C#: {0}", message); break;
	}
}

// C# keeps a pointer to it: ScriptBindings fills in the scene's functions later (only Log works without a game)
static ScriptNativeApi MakeNativeApi()
{
	ScriptNativeApi api {};
	api.Log = NativeLog;
	return api;
}

static ScriptNativeApi s_Native = MakeNativeApi();

// DescribeScripts' reports: a class, then its fields
static void ReportScriptField(void*, const ScriptFieldReport* report)
{
	const std::string className = ScriptUtf16ToUtf8(report->Class, report->ClassLength);
	if (!report->Name)
	{
		s_Classes.push_back({ className, {} });
		return;
	}
	if (s_Classes.empty() || s_Classes.back().Name != className)
	{
		return; // a field before its class: not from ScriptCore
	}
	ScriptFieldInfo field;
	field.Name = ScriptUtf16ToUtf8(report->Name, report->NameLength);
	field.Type = ScriptFieldType(report->Default.Type);
	field.TypeName = ScriptUtf16ToUtf8(report->TypeName, report->TypeNameLength);
	const std::string enumNames = ScriptUtf16ToUtf8(report->EnumNames, report->EnumNamesLength);
	for (size_t start = 0; field.Type == ScriptFieldType::Enum && report->EnumCount > 0 && start <= enumNames.size();)
	{
		const size_t end = std::min(enumNames.find('\n', start), enumNames.size());
		field.EnumNames.push_back(enumNames.substr(start, end - start));
		start = end + 1;
	}
	field.EnumValues.assign(report->EnumValues, report->EnumValues + (report->EnumValues ? report->EnumCount : 0));

	const ScriptValue& value = report->Default;
	field.Default.Type = field.Type;
	field.Default.Integer = value.Integer;
	field.Default.Number = value.Number;
	field.Default.Vector = value.Vector;
	field.Default.Entity = value.Entity;
	if (field.Type == ScriptFieldType::String || field.Type == ScriptFieldType::Prefab)
	{
		field.Default.Text = ScriptUtf16ToUtf8(value.Text, value.TextLength);
	}
	else if (field.Type == ScriptFieldType::Enum && !field.FindEnumName(value.Integer, field.Default.Text))
	{
		field.Default.Text = field.EnumNames.empty() ? std::string() : field.EnumNames.front();
	}
	s_Classes.back().Fields.push_back(std::move(field));
}

#if defined(MORAVA_SCRIPTING)

static void* LoadLibraryAt(const fs::path& path)
{
#if defined(_WIN32)
	return LoadLibraryW(path.c_str());
#else
	return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

template<typename T>
static T LibraryExport(void* library, const char* name)
{
#if defined(_WIN32)
	return reinterpret_cast<T>(reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name)));
#else
	return reinterpret_cast<T>(dlsym(library, name));
#endif
}

static bool IsInside(const fs::path& path, const fs::path& folder)
{
	std::error_code error;
	const fs::path relative = fs::weakly_canonical(path, error).lexically_relative(fs::weakly_canonical(folder, error));
	return !relative.empty() && *relative.begin() != "..";
}

// hostfxr's own messages (a runtime it can't find, a broken runtimeconfig.json), in the engine's log
static void HOSTFXR_CALLTYPE HostfxrError(const char_t* message)
{
	Log::GetLogger()->error("C# scripting: hostfxr: {0}", fs::path(message).u8string());
}

static std::string ResultCode(int result)
{
	char text[16];
	snprintf(text, sizeof(text), "0x%08X", static_cast<unsigned int>(result));
	return text;
}

static std::string ReadManagedString(int (*read)(char16_t*, int))
{
	char16_t buffer[1024];
	const int length = read(buffer, int(std::size(buffer)));
	return length > 0 ? ScriptUtf16ToUtf8(buffer, length) : std::string();
}

static bool StartRuntime()
{
	const auto start = std::chrono::steady_clock::now();
	const fs::path executable = Process::GetExecutablePath();
	const fs::path folder = executable.parent_path();
	const fs::path dotnetRoot = folder / "dotnet";
	const fs::path coreDll = folder / "Morava.ScriptCore.dll";
	const fs::path config = folder / "Morava.ScriptCore.runtimeconfig.json";

	std::error_code error;
	for (const fs::path& needed : { dotnetRoot, coreDll, config })
	{
		if (!fs::exists(needed, error))
		{
			Log::GetLogger()->error("C# scripting is not available: '{0}' not found next to the executable (the build puts it there)",
				needed.u8string());
			return false;
		}
	}

	// hostfxr from the private folder only
	get_hostfxr_parameters hostfxrParameters { sizeof(hostfxrParameters), nullptr, dotnetRoot.c_str() };
	char_t hostfxrBuffer[1024];
	size_t hostfxrSize = std::size(hostfxrBuffer);
	int result = get_hostfxr_path(hostfxrBuffer, &hostfxrSize, &hostfxrParameters);
	if (result != 0)
	{
		Log::GetLogger()->error("C# scripting is not available: no hostfxr in '{0}' ({1})", dotnetRoot.u8string(), ResultCode(result));
		return false;
	}
	const fs::path hostfxrPath = hostfxrBuffer;

	void* hostfxr = LoadLibraryAt(hostfxrPath);
	auto initialize = hostfxr ? LibraryExport<hostfxr_initialize_for_runtime_config_fn>(hostfxr, "hostfxr_initialize_for_runtime_config") : nullptr;
	auto getDelegate = hostfxr ? LibraryExport<hostfxr_get_runtime_delegate_fn>(hostfxr, "hostfxr_get_runtime_delegate") : nullptr;
	auto closeContext = hostfxr ? LibraryExport<hostfxr_close_fn>(hostfxr, "hostfxr_close") : nullptr;
	auto setErrorWriter = hostfxr ? LibraryExport<hostfxr_set_error_writer_fn>(hostfxr, "hostfxr_set_error_writer") : nullptr;
	if (!initialize || !getDelegate || !closeContext || !setErrorWriter)
	{
		Log::GetLogger()->error("C# scripting is not available: '{0}' could not be loaded", hostfxrPath.u8string());
		return false;
	}
	setErrorWriter(HostfxrError);

	// The runtime, from ScriptCore's runtimeconfig.json, with the private folder as dotnet_root
	hostfxr_initialize_parameters initializeParameters { sizeof(initializeParameters), executable.c_str(), dotnetRoot.c_str() };
	hostfxr_handle context = nullptr;
	result = initialize(config.c_str(), &initializeParameters, &context);
	if (result != 0 || !context)
	{
		Log::GetLogger()->error("C# scripting is not available: .NET could not be started from '{0}' ({1})", dotnetRoot.u8string(), ResultCode(result));
		if (context)
		{
			closeContext(context);
		}
		return false;
	}

	load_assembly_fn loadAssembly = nullptr;
	get_function_pointer_fn getFunctionPointer = nullptr;
	getDelegate(context, hdt_load_assembly, reinterpret_cast<void**>(&loadAssembly));
	getDelegate(context, hdt_get_function_pointer, reinterpret_cast<void**>(&getFunctionPointer));
	closeContext(context);
	setErrorWriter(nullptr);

	// ScriptCore into the default context (where debuggers and tools expect it); the two tables exchanged
	using InitializeFn = int (CORECLR_DELEGATE_CALLTYPE*)(const ScriptNativeApi*, ScriptManagedApi*);
	InitializeFn initializeCore = nullptr;
	result = loadAssembly && getFunctionPointer ? loadAssembly(coreDll.c_str(), nullptr, nullptr) : -1;
	if (result == 0)
	{
		result = getFunctionPointer(SCRIPT_HOST_STR("Morava.Interop, Morava.ScriptCore"), SCRIPT_HOST_STR("Initialize"),
			UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr, reinterpret_cast<void**>(&initializeCore));
	}
	if (result != 0 || !initializeCore || initializeCore(&s_Native, &s_Managed) != 0 || !s_Managed.GetField)
	{
		Log::GetLogger()->error("C# scripting is not available: '{0}' could not be loaded ({1})", coreDll.u8string(), ResultCode(result));
		s_Managed = {};
		return false;
	}

	s_RuntimeVersion = ReadManagedString(s_Managed.GetRuntimeVersion);
	s_RuntimeDirectory = fs::u8path(ReadManagedString(s_Managed.GetRuntimeDirectory));
	if (!IsInside(s_RuntimeDirectory, dotnetRoot))
	{
		Log::GetLogger()->warn("C# scripting: .NET runs from '{0}', not from the private folder '{1}'", s_RuntimeDirectory.u8string(), dotnetRoot.u8string());
	}

	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	Log::GetLogger()->info("C# scripting: .NET {0} from '{1}', Morava.ScriptCore loaded ({2} ms)",
		s_RuntimeVersion, s_RuntimeDirectory.u8string(), milliseconds);
	return true;
}

#endif

bool ScriptHost::Initialize()
{
	if (s_Initialized)
	{
		return s_Available;
	}
	s_Initialized = true;
#if defined(MORAVA_SCRIPTING)
	s_Available = StartRuntime();
#else
	Log::GetLogger()->info("C# scripting is not available: built with MORAVA_SCRIPTING=OFF");
#endif
	return s_Available;
}

void ScriptHost::Shutdown()
{
	if (s_Available && s_GameAssemblyLoaded)
	{
		UnloadGameAssembly();
	}
	s_Available = false; // the runtime itself stays until the process ends
}

bool ScriptHost::IsAvailable()
{
	return s_Available;
}

const std::string& ScriptHost::GetRuntimeVersion()
{
	return s_RuntimeVersion;
}

const fs::path& ScriptHost::GetRuntimeDirectory()
{
	return s_RuntimeDirectory;
}

bool ScriptHost::LoadGameAssembly(const fs::path& path)
{
	if (!s_Available)
	{
		return false;
	}
	// The file's time before it is read: a build that overwrites it while it is read is newer still, and loads next
	std::error_code timeError;
	const fs::file_time_type writeTime = fs::last_write_time(path, timeError);
	const std::u16string text = path.u16string();
	if (s_Managed.LoadGameAssembly(text.c_str(), int(text.size())) != 0)
	{
		Log::GetLogger()->error("C# scripting: the game assembly '{0}' could not be loaded", path.u8string());
		return false;
	}
	s_GameAssemblyLoaded = true;
	s_GameAssemblyPath = path;
	s_GameAssemblyWriteTime = writeTime;

	// Its script classes and their fields, for the Inspector and the script runtime
	s_Classes.clear();
	const int classCount = s_Managed.DescribeScripts(nullptr, ReportScriptField);
	std::sort(s_Classes.begin(), s_Classes.end(), [](const ScriptClass& a, const ScriptClass& b) { return a.Name < b.Name; });
	std::string names;
	for (const ScriptClass& scriptClass : s_Classes)
	{
		names += (names.empty() ? "" : ", ") + scriptClass.Name;
	}
	Log::GetLogger()->info("C# scripting: game assembly '{0}' loaded, {1} script class(es){2}{3}", path.u8string(),
		std::max(classCount, 0), names.empty() ? "" : ": ", names);
	return true;
}

ScriptUnloadResult ScriptHost::UnloadGameAssembly()
{
	if (!s_Available || !s_GameAssemblyLoaded)
	{
		return ScriptUnloadResult::Unloaded;
	}
	switch (s_Managed.UnloadGameAssembly())
	{
	case 0:
		s_GameAssemblyLoaded = false;
		s_GameAssemblyPath.clear();
		s_Classes.clear();
		Log::GetLogger()->info("C# scripting: game assembly unloaded");
		return ScriptUnloadResult::Unloaded;
	case -1:
		return ScriptUnloadResult::Refused; // C# has logged why
	default:
		// The context is gone from ScriptCore's side either way: a new one can be loaded, the old code stays in memory
		s_GameAssemblyLoaded = false;
		s_GameAssemblyPath.clear();
		s_Classes.clear();
		s_LeakedAssemblyCount++;
		Log::GetLogger()->error("C# scripting: the game assembly was unloaded, but something still holds it (a static, a delegate or a "
			"GCHandle into the game): its memory stays in use");
		return ScriptUnloadResult::StillAlive;
	}
}

bool ScriptHost::IsGameAssemblyLoaded()
{
	return s_GameAssemblyLoaded;
}

const fs::path& ScriptHost::GetGameAssemblyPath()
{
	return s_GameAssemblyPath;
}

fs::file_time_type ScriptHost::GetGameAssemblyWriteTime()
{
	return s_GameAssemblyWriteTime;
}

uint32_t ScriptHost::GetLeakedAssemblyCount()
{
	return s_LeakedAssemblyCount;
}

const std::vector<ScriptClass>& ScriptHost::GetClasses()
{
	return s_Classes;
}

const ScriptClass* ScriptHost::FindClass(const std::string& name)
{
	for (const ScriptClass& scriptClass : s_Classes)
	{
		if (scriptClass.Name == name)
		{
			return &scriptClass;
		}
	}
	return nullptr;
}

void* ScriptHost::CreateInstance(const std::string& className, uint64_t entity)
{
	if (!s_Available)
	{
		return nullptr;
	}
	const std::u16string name = ScriptUtf8ToUtf16(className);
	return s_Managed.CreateInstance(name.c_str(), int(name.size()), entity);
}

void ScriptHost::DestroyInstance(void* instance)
{
	if (s_Available && instance)
	{
		s_Managed.DestroyInstance(instance);
	}
}

bool ScriptHost::InvokeCreate(void* instance)
{
	return s_Available && instance && s_Managed.InvokeCreate(instance) == 0;
}

bool ScriptHost::InvokeUpdate(void* instance, float dt)
{
	return s_Available && instance && s_Managed.InvokeUpdate(instance, dt) == 0;
}

bool ScriptHost::InvokeFixedUpdate(void* instance, float dt)
{
	return s_Available && instance && s_Managed.InvokeFixedUpdate(instance, dt) == 0;
}

bool ScriptHost::InvokeCollision(void* instance, ScriptCollisionKind kind, uint64_t other, const glm::vec3& point, const glm::vec3& normal)
{
	return s_Available && instance && s_Managed.InvokeCollision(instance, int(kind), other, &point, &normal) == 0;
}

bool ScriptHost::InvokeAnimationEvent(void* instance, const std::string& name, const std::string& state)
{
	if (!s_Available || !instance)
	{
		return false;
	}
	const std::u16string name16 = ScriptUtf8ToUtf16(name);
	const std::u16string state16 = ScriptUtf8ToUtf16(state);
	return s_Managed.InvokeAnimationEvent(instance, name16.c_str(), int(name16.size()), state16.c_str(), int(state16.size())) == 0;
}

bool ScriptHost::SetField(void* instance, const ScriptFieldInfo& field, const ScriptFieldValue& value)
{
	if (!s_Available || !instance || value.Type != field.Type)
	{
		return false;
	}
	ScriptValue native;
	native.Type = int32_t(value.Type);
	native.Integer = value.Integer;
	native.Number = value.Number;
	native.Vector = value.Vector;
	native.Entity = value.Entity;
	std::u16string text;
	if (value.Type == ScriptFieldType::String || value.Type == ScriptFieldType::Prefab)
	{
		text = ScriptUtf8ToUtf16(value.Text);
		native.Text = text.data();
		native.TextLength = int32_t(text.size());
	}
	else if (value.Type == ScriptFieldType::Enum && !field.FindEnumValue(value.Text, native.Integer))
	{
		Log::GetLogger()->warn("C# scripting: {0} has no value '{1}' (the field {2} keeps its own)", field.TypeName, value.Text, field.Name);
		return false;
	}
	const std::u16string name = ScriptUtf8ToUtf16(field.Name);
	return s_Managed.SetField(instance, name.c_str(), int(name.size()), &native) == 0;
}

bool ScriptHost::GetField(void* instance, const ScriptFieldInfo& field, ScriptFieldValue& value)
{
	if (!s_Available || !instance)
	{
		return false;
	}
	const std::u16string name = ScriptUtf8ToUtf16(field.Name);
	std::u16string text(256, u'\0');
	ScriptValue native;
	native.Text = text.data();
	native.TextCapacity = int32_t(text.size());
	if (s_Managed.GetField(instance, name.c_str(), int(name.size()), &native) != 0)
	{
		return false;
	}
	const bool hasText = native.Type == int32_t(ScriptFieldType::String) || native.Type == int32_t(ScriptFieldType::Prefab);
	if (hasText && native.TextLength > native.TextCapacity)
	{
		// A longer string: again, with room for it
		text.assign(size_t(native.TextLength), u'\0');
		native.Text = text.data();
		native.TextCapacity = int32_t(text.size());
		if (s_Managed.GetField(instance, name.c_str(), int(name.size()), &native) != 0)
		{
			return false;
		}
	}
	value = ScriptFieldValue();
	value.Type = ScriptFieldType(native.Type);
	value.Integer = native.Integer;
	value.Number = native.Number;
	value.Vector = native.Vector;
	value.Entity = native.Entity;
	if (hasText)
	{
		value.Text = ScriptUtf16ToUtf8(native.Text, std::min(native.TextLength, native.TextCapacity));
	}
	else if (value.Type == ScriptFieldType::Enum && !field.FindEnumName(native.Integer, value.Text))
	{
		value.Text = std::to_string(native.Integer); // a value the enum doesn't name (a cast in the script)
	}
	return true;
}

ScriptNativeApi& ScriptHost::GetNativeApi()
{
	return s_Native;
}
