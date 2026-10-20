#pragma once

#include <string>
#include <vector>


// One entry of a file dialog's list of file types
struct FileDialogFilter
{
	std::string Name;       // shown in the dialog, e.g. "Images"
	std::string Extensions; // without dots, separated by commas, e.g. "png,jpg,jpeg"
};

/**
 * The operating system's own file dialogs, on Windows, Linux and macOS (through Native File Dialog Extended). All text is
 * UTF-8. A dialog blocks until it is closed; it returns the chosen file's full path, or an empty string when it was
 * canceled. The working directory is not changed.
 *
 * filters: the file types to choose from (the first is the preselected one); every dialog also offers all files.
 * initialDirectory (optional): the folder the dialog starts in.
 */
class FileDialogs
{
public:
	static std::string OpenFile(const std::vector<FileDialogFilter>& filters = {}, const std::string& initialDirectory = {});
	// A new name can be typed; an existing file is overwritten only after the dialog has asked. A typed name without an
	// extension gets the first one of the first filter.
	static std::string SaveFile(const std::vector<FileDialogFilter>& filters = {}, const std::string& initialDirectory = {});
	// A folder (one can be made in the dialog). title (optional): the dialog's title, saying what the folder is for
	static std::string PickFolder(const std::string& title = {}, const std::string& initialDirectory = {});

	// The filters of a Windows filter string, as the older scenes pass them: pairs of name and patterns separated and
	// ended by null characters, e.g. "Images\0*.png;*.jpg\0All\0*.*\0". A single pattern alone ("*.hdr") is also taken.
	static std::vector<FileDialogFilter> FiltersFromWindowsString(const char* filter);
	static std::vector<FileDialogFilter> FiltersFromWindowsString(const wchar_t* filter);

};
