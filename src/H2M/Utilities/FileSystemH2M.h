#pragma once

#include <functional>

namespace H2M
{

	enum class FileSystemActionH2M
	{
		Added, Rename, Modified, Delete
	};

	struct FileSystemChangedEventH2M
	{
		FileSystemActionH2M Action;
		std::string FilePath;
		std::string OldName;
		std::string NewName;
		bool IsDirectory;
	};

	class FileSystemH2M
	{
	public:
		static bool CreateFolder(const std::string& filepath);
		static bool Exists(const std::string& filepath);
		static std::string Rename(const std::string& filepath, const std::string& newName);
		static bool DeleteFile(const std::string& filepath);
		static bool MoveFile(const std::string& filepath, const std::string& dest);

	public:
		using FileSystemChangedCallbackFn = std::function<void(FileSystemChangedEventH2M)>;

		static void SetChangeCallback(const FileSystemChangedCallbackFn& callback);
		static void StartWatching();
		static void StopWatching();

	private:
		static unsigned long Watch(void* param);

	private:
		static FileSystemChangedCallbackFn s_Callback;
	};
}
