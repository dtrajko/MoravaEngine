#include "CookFiles.h"

#include "Core/EnginePaths.h"
#include "Project/AssetRegistry.h"
#include "Project/Project.h"

#include <cstring>
#include <fstream>
#include <system_error>


namespace fs = std::filesystem;

namespace CookFiles
{
	std::string GetCacheFolder()
	{
		if (const Project* project = Project::GetOpen())
		{
			return project->GetDirectory() + "/Library/Cooked";
		}
		return RESOURCES_DIR "/Cache/Cooked";
	}

	uint64_t HashBytes(const uint8_t* bytes, size_t size)
	{
		// FNV-1a on eight bytes at a time, with a shift for the high bits to reach the low ones: fast enough in a Debug build
		static constexpr uint64_t Prime = 0x100000001b3ull;
		uint64_t hash = 0xcbf29ce484222325ull ^ size;
		size_t i = 0;
		for (; i + 8 <= size; i += 8)
		{
			uint64_t word;
			memcpy(&word, bytes + i, 8);
			hash = (hash ^ word) * Prime;
			hash ^= hash >> 29;
		}
		for (; i < size; i++)
		{
			hash = (hash ^ bytes[i]) * Prime;
		}
		return hash;
	}

	std::string GetCookedName(const std::string& path)
	{
		const AssetID id = AssetRegistry::FindID(path);
		if (id != 0)
		{
			return std::to_string(id);
		}
		std::error_code error;
		const std::string file = fs::weakly_canonical(fs::u8path(path), error).generic_u8string();
		return "path_" + std::to_string(HashBytes(reinterpret_cast<const uint8_t*>(file.data()), file.size()));
	}

	bool ReadWholeFile(const fs::path& path, std::vector<uint8_t>& bytes)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			return false;
		}
		const std::streamoff size = file.tellg();
		if (size < 0)
		{
			return false;
		}
		bytes.resize((size_t)size);
		file.seekg(0);
		return size == 0 || (bool)file.read(reinterpret_cast<char*>(bytes.data()), size);
	}

	bool WriteFileSafely(const fs::path& path, const uint8_t* bytes, size_t size)
	{
		std::error_code error;
		fs::create_directories(path.parent_path(), error);

		// Under another name first: a run cut short leaves no half a file under the real one
		fs::path temporary = path;
		temporary += ".tmp";
		bool written = false;
		{
			std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
			file.write(reinterpret_cast<const char*>(bytes), (std::streamsize)size);
			written = file.good();
		}
		if (written)
		{
			fs::rename(temporary, path, error);
			written = !error;
		}
		if (!written)
		{
			fs::remove(temporary, error);
		}
		return written;
	}

	bool GetFileState(const fs::path& path, uint64_t& size, int64_t& writeTime)
	{
		std::error_code error;
		size = fs::file_size(path, error);
		if (error)
		{
			return false;
		}
		const fs::file_time_type time = fs::last_write_time(path, error);
		if (error)
		{
			return false;
		}
		writeTime = (int64_t)time.time_since_epoch().count();
		return true;
	}

	void CountFiles(const std::string& extension, uint32_t& files, uint64_t& bytes)
	{
		files = 0;
		bytes = 0;
		std::error_code error;
		for (fs::directory_iterator it(fs::u8path(GetCacheFolder()), error), end; !error && it != end; it.increment(error))
		{
			std::error_code sizeError;
			if (it->path().extension() == extension && it->is_regular_file(sizeError))
			{
				files++;
				bytes += it->file_size(sizeError);
			}
		}
	}

	uint32_t RemoveFiles(const std::string& extension)
	{
		std::error_code error;
		std::vector<fs::path> found;
		for (fs::directory_iterator it(fs::u8path(GetCacheFolder()), error), end; !error && it != end; it.increment(error))
		{
			if (it->path().extension() == extension)
			{
				found.push_back(it->path());
			}
		}
		uint32_t removed = 0;
		for (const fs::path& file : found)
		{
			std::error_code removeError;
			if (fs::remove(file, removeError))
			{
				removed++;
			}
		}
		return removed;
	}
}
