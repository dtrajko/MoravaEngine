// Visual Studio as the scripts' code editor (docs/engine/Game_Engine_Runtime_Guide.html, 4.7): a script opens in the Visual
// Studio that has the project's .csproj open, found through Visual Studio's automation interface (DTE, in the system's
// running object table), not in the one used last (devenv /Edit, which can be the engine's C++ solution). When none has
// it, one is started with the .csproj and the script opens in it once the project is loaded. Unity's Visual Studio
// integration works the same way. Windows only; elsewhere Visual Studio isn't offered.


#include "Core/Log.h"
#include "Core/Process.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <objbase.h>
	#include <oleauto.h>
	#include <wrl/client.h>
	#include <chrono>
	#include <vector>
#endif


#include "EditorState.h"


namespace fs = std::filesystem;

// The worker's requests and its result: the latest request wins (a second click while Visual Studio starts opens the
// second file in it); the result is shown by UpdateVisualStudio, on the editor's thread
static std::mutex s_Mutex;
static bool s_WorkerRunning = false;
static bool s_RequestPending = false;
static std::string s_Devenv, s_Project, s_File;
static int s_Line = 1;
static std::string s_ResultText;
static bool s_ResultIsError = false;

#if defined(_WIN32)

using Microsoft::WRL::ComPtr;

// Visual Studio rejects calls while it is busy (loading a solution, a modal dialog): COM asks this filter, which waits
// and tries again, for up to 30 seconds
class RetryMessageFilter : public IMessageFilter
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override
	{
		if (riid == IID_IUnknown || riid == IID_IMessageFilter)
		{
			*object = static_cast<IMessageFilter*>(this);
			return S_OK;
		}
		*object = nullptr;
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef() override { return 2; } // lives as long as its thread
	ULONG STDMETHODCALLTYPE Release() override { return 1; }
	DWORD STDMETHODCALLTYPE HandleInComingCall(DWORD, HTASK, DWORD, LPINTERFACEINFO) override { return SERVERCALL_ISHANDLED; }
	DWORD STDMETHODCALLTYPE RetryRejectedCall(HTASK, DWORD elapsed, DWORD rejectType) override
	{
		return (rejectType == SERVERCALL_RETRYLATER && elapsed < 30000) ? 100 : (DWORD)-1;
	}
	DWORD STDMETHODCALLTYPE MessagePending(HTASK, DWORD, DWORD) override { return PENDINGMSG_WAITDEFPROCESS; }
};

// A VARIANT that clears itself
struct Variant
{
	VARIANT Value;
	Variant() { VariantInit(&Value); }
	explicit Variant(const std::wstring& text) { VariantInit(&Value); Value.vt = VT_BSTR; Value.bstrVal = SysAllocString(text.c_str()); }
	explicit Variant(int number) { VariantInit(&Value); Value.vt = VT_I4; Value.lVal = number; }
	explicit Variant(bool flag) { VariantInit(&Value); Value.vt = VT_BOOL; Value.boolVal = flag ? VARIANT_TRUE : VARIANT_FALSE; }
	~Variant() { VariantClear(&Value); }
	Variant(const Variant&) = delete;
	Variant& operator=(const Variant&) = delete;
};

// A property or method of an automation object by name (late bound: works with every Visual Studio version)
static HRESULT Call(IDispatch* object, const wchar_t* name, WORD flags, VARIANT* result, std::vector<VARIANT*> arguments = {})
{
	DISPID id = 0;
	LPOLESTR names[] = { const_cast<LPOLESTR>(name) };
	HRESULT hr = object->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &id);
	if (FAILED(hr))
	{
		return hr;
	}
	std::vector<VARIANT> reversed; // IDispatch takes them last first
	for (auto it = arguments.rbegin(); it != arguments.rend(); ++it)
	{
		reversed.push_back(**it);
	}
	DISPPARAMS parameters = { reversed.empty() ? nullptr : reversed.data(), nullptr, (UINT)reversed.size(), 0 };
	return object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, flags, &parameters, result, nullptr, nullptr);
}

static ComPtr<IDispatch> GetDispatch(IDispatch* object, const wchar_t* name, std::vector<VARIANT*> arguments = {})
{
	Variant result;
	ComPtr<IDispatch> out;
	if (SUCCEEDED(Call(object, name, DISPATCH_PROPERTYGET | DISPATCH_METHOD, &result.Value, arguments)) &&
		result.Value.vt == VT_DISPATCH && result.Value.pdispVal)
	{
		out = result.Value.pdispVal;
	}
	return out;
}

static std::wstring GetText(IDispatch* object, const wchar_t* name)
{
	Variant result;
	if (SUCCEEDED(Call(object, name, DISPATCH_PROPERTYGET, &result.Value)) && result.Value.vt == VT_BSTR && result.Value.bstrVal)
	{
		return result.Value.bstrVal;
	}
	return std::wstring();
}

static long GetNumber(IDispatch* object, const wchar_t* name)
{
	Variant result;
	if (SUCCEEDED(Call(object, name, DISPATCH_PROPERTYGET, &result.Value)) &&
		SUCCEEDED(VariantChangeType(&result.Value, &result.Value, 0, VT_I4)))
	{
		return result.Value.lVal;
	}
	return 0;
}

// Its solution's projects (the top-level ones: a script project isn't put in a solution folder) has this .csproj
static bool HasProject(IDispatch* dte, const fs::path& csproj)
{
	ComPtr<IDispatch> solution = GetDispatch(dte, L"Solution");
	ComPtr<IDispatch> projects = solution ? GetDispatch(solution.Get(), L"Projects") : nullptr;
	if (!projects)
	{
		return false;
	}
	const long count = GetNumber(projects.Get(), L"Count");
	for (long i = 1; i <= count; i++)
	{
		Variant index(static_cast<int>(i));
		ComPtr<IDispatch> project = GetDispatch(projects.Get(), L"Item", { &index.Value });
		const std::wstring path = project ? GetText(project.Get(), L"FullName") : std::wstring();
		std::error_code error;
		if (!path.empty() && fs::equivalent(fs::path(path), csproj, error))
		{
			return true;
		}
	}
	return false;
}

// The running Visual Studio that has the project (every one registers "!VisualStudio.DTE.<version>:<process id>")
static ComPtr<IDispatch> FindVisualStudio(const fs::path& csproj)
{
	ComPtr<IRunningObjectTable> table;
	ComPtr<IEnumMoniker> monikers;
	if (FAILED(GetRunningObjectTable(0, &table)) || FAILED(table->EnumRunning(&monikers)))
	{
		return nullptr;
	}
	ComPtr<IMoniker> moniker;
	while (monikers->Next(1, &moniker, nullptr) == S_OK)
	{
		ComPtr<IBindCtx> context;
		LPOLESTR name = nullptr;
		bool isVisualStudio = false;
		if (SUCCEEDED(CreateBindCtx(0, &context)) && SUCCEEDED(moniker->GetDisplayName(context.Get(), nullptr, &name)))
		{
			isVisualStudio = std::wstring(name).rfind(L"!VisualStudio.DTE.", 0) == 0;
			CoTaskMemFree(name);
		}
		ComPtr<IUnknown> object;
		ComPtr<IDispatch> dte;
		if (isVisualStudio && SUCCEEDED(table->GetObject(moniker.Get(), &object)) && SUCCEEDED(object.As(&dte)) &&
			HasProject(dte.Get(), csproj))
		{
			return dte;
		}
		moniker.Reset();
	}
	return nullptr;
}

// The file at the line (none: only the window to the front)
static bool ShowInVisualStudio(IDispatch* dte, const fs::path& file, int line)
{
	if (!file.empty())
	{
		ComPtr<IDispatch> operations = GetDispatch(dte, L"ItemOperations");
		Variant path(file.wstring());
		Variant opened;
		if (!operations || FAILED(Call(operations.Get(), L"OpenFile", DISPATCH_METHOD, &opened.Value, { &path.Value })))
		{
			return false;
		}
		ComPtr<IDispatch> document = GetDispatch(dte, L"ActiveDocument");
		ComPtr<IDispatch> selection = document ? GetDispatch(document.Get(), L"Selection") : nullptr;
		Variant lineNumber(line), select(false);
		if (selection)
		{
			Call(selection.Get(), L"GotoLine", DISPATCH_METHOD, nullptr, { &lineNumber.Value, &select.Value });
		}
	}
	ComPtr<IDispatch> window = GetDispatch(dte, L"MainWindow");
	if (window)
	{
		Call(window.Get(), L"Activate", DISPATCH_METHOD, nullptr);
		// The editor is the foreground program (its button was just clicked), so Windows lets it bring Visual Studio up
		HWND handle = (HWND)(LONG_PTR)GetNumber(window.Get(), L"HWnd");
		if (handle)
		{
			if (IsIconic(handle))
			{
				ShowWindow(handle, SW_RESTORE);
			}
			SetForegroundWindow(handle);
		}
	}
	return true;
}

static void SetResult(const std::string& text, bool isError)
{
	std::lock_guard<std::mutex> lock(s_Mutex);
	s_ResultText = text;
	s_ResultIsError = isError;
}

// One request: the Visual Studio that has the project, else a new one with it (waited for: up to 2 minutes to load)
static void HandleRequest(const std::string& devenv, const fs::path& csproj, const fs::path& file, int line)
{
	ComPtr<IDispatch> dte = FindVisualStudio(csproj);
	if (!dte)
	{
		std::string error;
		if (!Process::Start(devenv, { csproj.u8string() }, error))
		{
			Log::GetLogger()->error("Code editor: {0}", error);
			SetResult("Visual Studio could not be started: the log says why", true);
			return;
		}
		Log::GetLogger()->info("Code editor: Visual Studio started with '{0}'", csproj.generic_u8string());
		const auto start = std::chrono::steady_clock::now();
		while (!(dte = FindVisualStudio(csproj)))
		{
			if (std::chrono::steady_clock::now() - start > std::chrono::minutes(2))
			{
				SetResult("Visual Studio didn't open the scripts' project in 2 minutes", true);
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		if (file.empty())
		{
			return; // it comes up by itself
		}
	}
	if (!ShowInVisualStudio(dte.Get(), file, line))
	{
		SetResult("Visual Studio could not open '" + file.filename().u8string() + "'", true);
	}
}

static void Worker()
{
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	RetryMessageFilter filter;
	IMessageFilter* previous = nullptr;
	CoRegisterMessageFilter(&filter, &previous);
	for (;;)
	{
		std::string devenv, project, file;
		int line = 1;
		{
			std::lock_guard<std::mutex> lock(s_Mutex);
			if (!s_RequestPending)
			{
				s_WorkerRunning = false;
				break;
			}
			s_RequestPending = false;
			devenv = s_Devenv;
			project = s_Project;
			file = s_File;
			line = s_Line;
		}
		HandleRequest(devenv, fs::u8path(project), file.empty() ? fs::path() : fs::u8path(file), line);
	}
	CoRegisterMessageFilter(previous, nullptr);
	CoUninitialize();
}

#endif

bool OpenInVisualStudio(const std::string& devenv, const std::string& csproj, const std::string& file, int line)
{
#if defined(_WIN32)
	std::lock_guard<std::mutex> lock(s_Mutex);
	s_Devenv = devenv;
	s_Project = csproj;
	s_File = file;
	s_Line = line;
	s_RequestPending = true;
	if (!s_WorkerRunning)
	{
		s_WorkerRunning = true;
		std::thread(Worker).detach(); // a request waits at most for Visual Studio to start
	}
	return true;
#else
	(void)devenv; (void)csproj; (void)file; (void)line;
	return false;
#endif
}

void UpdateVisualStudio()
{
	std::string text;
	bool isError = false;
	{
		std::lock_guard<std::mutex> lock(s_Mutex);
		if (s_ResultText.empty())
		{
			return;
		}
		std::swap(text, s_ResultText);
		isError = s_ResultIsError;
	}
	if (isError)
	{
		Log::GetLogger()->error("Code editor: {0}", text);
	}
	ShowNotification(text, isError);
}
