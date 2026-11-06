#include "AssetFiles.h"

#include "CookFiles.h"

#include "Core/Log.h"
#include "Project/Project.h"

#include <filesystem>
#include <fstream>
#include <memory>


#include <mutex>

namespace fs = std::filesystem;

namespace AssetFiles
{
	static std::unique_ptr<AssetPack> s_Pack;
	static std::string s_PackFile;
	static std::mutex s_FailedFileMutex;
	static std::string s_LastFailedFile;
	static std::function<void(const std::string&)> s_DamagedFileHandler;

	void NoteFailedFile(const std::string& path)
	{
		std::lock_guard<std::mutex> lock(s_FailedFileMutex);
		s_LastFailedFile = path;
	}

	std::string GetLastFailedFile()
	{
		// Asked in a crash, which may have stopped a thread here
		std::unique_lock<std::mutex> lock(s_FailedFileMutex, std::try_to_lock);
		return lock.owns_lock() ? s_LastFailedFile : std::string();
	}

	void SetDamagedFileHandler(const std::function<void(const std::string& file)>& handler)
	{
		s_DamagedFileHandler = handler;
	}

	bool MountPack(const std::string& file, std::string& error)
	{
		auto in = std::make_unique<std::ifstream>(fs::u8path(file), std::ios::binary);
		if (!*in)
		{
			error = "the file can't be opened";
			return false;
		}
		auto pack = std::make_unique<AssetPack>();
		if (!pack->Open(std::move(in), error))
		{
			return false;
		}
		std::error_code fileError;
		s_PackFile = fs::absolute(fs::u8path(file), fileError).lexically_normal().generic_u8string();
		s_Pack = std::move(pack);
		Log::GetLogger()->info("Assets: pack '{0}' mounted, {1} entries", s_PackFile, s_Pack->GetEntries().size());
		return true;
	}

	bool IsPackMounted() { return s_Pack != nullptr; }
	const AssetPack* GetPack() { return s_Pack.get(); }
	const std::string& GetPackFile() { return s_PackFile; }

	static bool ReadEntry(const AssetPack::Entry& entry, std::vector<uint8_t>& bytes)
	{
		std::string error;
		if (!s_Pack->Read(entry, bytes, error))
		{
			Log::GetLogger()->error("Assets: '{0}' can't be read: {1}", entry.Path, error);
			NoteFailedFile(entry.Path);
			if (s_DamagedFileHandler)
			{
				s_DamagedFileHandler(entry.Path);
			}
			return false;
		}
		return true;
	}

	bool Exists(const std::string& path)
	{
		if (s_Pack)
		{
			return s_Pack->Contains(ProjectPaths::ToStored(path));
		}
		std::error_code error;
		return fs::exists(fs::u8path(path), error);
	}

	bool Read(const std::string& path, std::vector<uint8_t>& bytes)
	{
		if (s_Pack)
		{
			const std::string stored = ProjectPaths::ToStored(path);
			const AssetPack::Entry* entry = s_Pack->Find(stored, AssetPack::Kind::File);
			if (!entry)
			{
				NoteFailedFile(stored);
				return false;
			}
			return ReadEntry(*entry, bytes);
		}
		return CookFiles::ReadWholeFile(fs::u8path(path), bytes);
	}

	bool ReadText(const std::string& path, std::string& text)
	{
		std::vector<uint8_t> bytes;
		if (!Read(path, bytes))
		{
			return false;
		}
		text.assign(bytes.begin(), bytes.end());
		return true;
	}

	YAML::Node LoadYaml(const std::string& path)
	{
		if (!s_Pack)
		{
			return YAML::LoadFile(path);
		}
		std::string text;
		if (!ReadText(path, text))
		{
			throw YAML::BadFile(path);
		}
		return YAML::Load(text);
	}

	bool ReadCooked(const std::string& path, AssetPack::Kind kind, std::vector<uint8_t>& bytes)
	{
		if (!s_Pack)
		{
			return false;
		}
		const AssetPack::Entry* entry = s_Pack->Find(ProjectPaths::ToStored(path), kind);
		return entry && ReadEntry(*entry, bytes);
	}
}
