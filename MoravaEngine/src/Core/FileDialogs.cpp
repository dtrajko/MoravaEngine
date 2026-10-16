#include "Core/FileDialogs.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Util.h"

// The dialog's owner window: the handle of the GLFW window, in the form the platform's dialogs take
#if defined(_WIN32)
	#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
	#define GLFW_EXPOSE_NATIVE_COCOA
#else
	#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <nfd.h>
#define Window X11Window // X11 typedefs a Window too: renamed here, the engine keeps its own
#include <nfd_glfw3.h>
#undef Window

#include <filesystem>


namespace {

	// The dialog library, set up for the time of one dialog
	struct DialogSession
	{
		bool Ready = false;

		DialogSession()
		{
			Ready = NFD_Init() == NFD_OKAY;
			if (!Ready)
			{
				Log::GetLogger()->error("File dialog: {0}", NFD_GetError() ? NFD_GetError() : "could not be set up");
			}
		}

		~DialogSession()
		{
			if (Ready)
			{
				NFD_Quit();
			}
		}
	};

	// The application's window as the dialog's owner: the dialog stays in front of it. None for a window that isn't a
	// GLFW one (DirectX 11): the dialog then stands on its own.
	nfdwindowhandle_t GetOwnerWindow()
	{
		nfdwindowhandle_t owner = {};
		Window* window = Application::Get() ? Application::Get()->GetWindow() : nullptr;
		GLFWwindow* glfwWindow = window ? window->GetHandle() : nullptr;
		if (glfwWindow)
		{
			NFD_GetNativeWindowFromGLFWWindow(glfwWindow, &owner);
		}
		return owner;
	}

	// The filters in the library's form (pointers into the given filters) without the ones for all files: the library
	// adds its own entry for those
	std::vector<nfdu8filteritem_t> ToItems(const std::vector<FileDialogFilter>& filters)
	{
		std::vector<nfdu8filteritem_t> items;
		for (const FileDialogFilter& filter : filters)
		{
			if (!filter.Extensions.empty())
			{
				items.push_back({ filter.Name.c_str(), filter.Extensions.c_str() });
			}
		}
		return items;
	}

	// "*.png;*.jpg" -> "png,jpg"; a pattern for every file ("*.*", "*") gives nothing
	std::string ExtensionsFromPatterns(const std::string& patterns)
	{
		std::string extensions;
		size_t start = 0;
		while (start <= patterns.size())
		{
			size_t end = patterns.find(';', start);
			if (end == std::string::npos)
			{
				end = patterns.size();
			}
			std::string pattern = patterns.substr(start, end - start);
			const size_t dot = pattern.rfind('.');
			std::string extension = dot == std::string::npos ? std::string() : pattern.substr(dot + 1);
			if (!extension.empty() && extension.find('*') == std::string::npos)
			{
				extensions += (extensions.empty() ? "" : ",") + extension;
			}
			start = end + 1;
		}
		return extensions;
	}

	// Of a null-separated list: the string at the pointer, which then moves behind it
	template <typename Char>
	std::basic_string<Char> NextString(const Char*& text)
	{
		std::basic_string<Char> string(text);
		text += string.size() + 1;
		return string;
	}

	template <typename Char, typename ToUtf8>
	std::vector<FileDialogFilter> ParseWindowsFilter(const Char* filter, ToUtf8 toUtf8)
	{
		std::vector<FileDialogFilter> filters;
		if (!filter)
		{
			return filters;
		}
		const std::string first = toUtf8(NextString(filter));
		if (first.empty())
		{
			return filters;
		}
		if (first[0] == '*')
		{
			// A pattern alone, without a name and without the list's end behind it
			const std::string extensions = ExtensionsFromPatterns(first);
			if (!extensions.empty())
			{
				filters.push_back({ first, extensions });
			}
			return filters;
		}
		std::string name = first;
		while (!name.empty())
		{
			const std::string patterns = toUtf8(NextString(filter));
			if (patterns.empty())
			{
				break;
			}
			filters.push_back({ name, ExtensionsFromPatterns(patterns) });
			name = toUtf8(NextString(filter));
		}
		return filters;
	}

}

std::string FileDialogs::OpenFile(const std::vector<FileDialogFilter>& filters, const std::string& initialDirectory)
{
	DialogSession session;
	if (!session.Ready)
	{
		return std::string();
	}
	const std::vector<nfdu8filteritem_t> items = ToItems(filters);
	nfdopendialogu8args_t arguments = {};
	arguments.filterList = items.empty() ? nullptr : items.data();
	arguments.filterCount = (nfdfiltersize_t)items.size();
	arguments.defaultPath = initialDirectory.empty() ? nullptr : initialDirectory.c_str();
	arguments.parentWindow = GetOwnerWindow();

	std::string path;
	nfdu8char_t* chosen = nullptr;
	const nfdresult_t result = NFD_OpenDialogU8_With(&chosen, &arguments);
	if (result == NFD_OKAY && chosen)
	{
		path = chosen;
		NFD_FreePathU8(chosen);
	}
	else if (result == NFD_ERROR)
	{
		Log::GetLogger()->error("File dialog: {0}", NFD_GetError() ? NFD_GetError() : "failed");
	}
	return path;
}

std::string FileDialogs::SaveFile(const std::vector<FileDialogFilter>& filters, const std::string& initialDirectory)
{
	DialogSession session;
	if (!session.Ready)
	{
		return std::string();
	}
	const std::vector<nfdu8filteritem_t> items = ToItems(filters);
	nfdsavedialogu8args_t arguments = {};
	arguments.filterList = items.empty() ? nullptr : items.data();
	arguments.filterCount = (nfdfiltersize_t)items.size();
	arguments.defaultPath = initialDirectory.empty() ? nullptr : initialDirectory.c_str();
	arguments.parentWindow = GetOwnerWindow();

	std::string path;
	nfdu8char_t* chosen = nullptr;
	const nfdresult_t result = NFD_SaveDialogU8_With(&chosen, &arguments);
	if (result == NFD_OKAY && chosen)
	{
		path = chosen;
		NFD_FreePathU8(chosen);
	}
	else if (result == NFD_ERROR)
	{
		Log::GetLogger()->error("File dialog: {0}", NFD_GetError() ? NFD_GetError() : "failed");
	}

	// A name typed without an extension: the first filter's first one (not every platform's dialog adds it)
	if (!path.empty() && !items.empty() && !std::filesystem::u8path(path).has_extension())
	{
		const std::string extensions = items.front().spec;
		path += "." + extensions.substr(0, extensions.find(','));
	}
	return path;
}

std::vector<FileDialogFilter> FileDialogs::FiltersFromWindowsString(const char* filter)
{
	return ParseWindowsFilter(filter, [](const std::string& text) { return text; });
}

std::vector<FileDialogFilter> FileDialogs::FiltersFromWindowsString(const wchar_t* filter)
{
	return ParseWindowsFilter(filter, [](const std::wstring& text) { return Util::ToUtf8(text); });
}
