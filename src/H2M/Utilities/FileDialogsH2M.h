/**
 * File name renamed from PlatformUtils to FileDialogs (easier to find on disk)
 */

#pragma once

#include <string>


namespace H2M
{

	class FileDialogsH2M
	{
	public:
		// These return empty strings if cancelled
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter);

	};

}
