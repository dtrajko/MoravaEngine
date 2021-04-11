#include "../../Utils/PlatformUtils.h"

#include "Core/Application.h"

#include <GLFW/glfw3.h>

#if defined(HZ_PLATFORM_WINDOWS)
	#include <commdlg.h>

	#define GLFW_EXPOSE_NATIVE_WIN32
	#include <GLFW/glfw3native.h>
#endif


namespace Hazel {

	std::string FileDialogs::OpenFile(const char* filter)
	{

#if defined(HZ_PLATFORM_WINDOWS)

		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get()->GetWindow()->GetHandle());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE) {
			return ofn.lpstrFile;
		}

#endif

		return std::string();
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{

#if defined(HZ_PLATFORM_WINDOWS)

		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(Application::Get()->GetWindow()->GetHandle());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE) {
			return ofn.lpstrFile;
		}

#endif

		return std::string();
	}

}
