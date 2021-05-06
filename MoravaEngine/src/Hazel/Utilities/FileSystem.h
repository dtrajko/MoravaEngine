#pragma once

#include <functional>

namespace Hazel {

	enum class FileSystemAction
	{
		Added, Rename, Modified, Delete
	};

	struct FileSystemChangedEvent
	{
		FileSystemAction Action;
		std::string FilePath;
		std::string OldName;
		std::string NewName;
		bool IsDirectory;
	};

	class FileSystem
	{
	public:
		static bool CreateFolder(const std::string& filepath);
		static bool Exists(const std::string& filepath);
		static std::string Rename(const std::string& filepath, const std::string& newName);
		static bool DeleteFile(const std::string& filepath);
		static bool MoveFile(const std::string& filepath, const std::string& dest);

	public:
		using FileSystemChangedCallbackFn = std::function<void(FileSystemChangedEvent)>;

		static void SetChangeCallback(const FileSystemChangedCallbackFn& callback);
		static void StartWatching();
		static void StopWatching();

	private:
		static unsigned long Watch(void* param);

	private:
		static FileSystemChangedCallbackFn s_Callback;
	};
}
