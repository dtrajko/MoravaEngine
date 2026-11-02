#include "H2M/Utilities/FileDialogsH2M.h"

#include "Core/FileDialogs.h"


namespace H2M {

	// filter: a Windows filter string ("Hazel Scene (*.hazel)\0*.hazel\0"). The dialogs themselves: FileDialogs.

	std::string FileDialogsH2M::OpenFile(const char* filter)
	{
		return FileDialogs::OpenFile(FileDialogs::FiltersFromWindowsString(filter));
	}

	std::string FileDialogsH2M::SaveFile(const char* filter)
	{
		return FileDialogs::SaveFile(FileDialogs::FiltersFromWindowsString(filter));
	}

}
