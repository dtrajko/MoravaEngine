#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>


/**
 * What the cookers share (ModelCook, TextureCook): the cache folder, a hash, reading a file whole and writing one so that a
 * run cut short leaves no half a file under the real name.
 */
namespace CookFiles
{
	// <Project>/Library/Cooked, or Resources/Cache/Cooked without a project
	std::string GetCacheFolder();

	uint64_t HashBytes(const uint8_t* bytes, size_t size);

	// The name of a cooked file without its extension: the asset's ID, so that a moved file keeps it, or for a file without
	// one (an engine file nothing saved refers to) a hash of its path. On the main thread: it asks the asset registry
	std::string GetCookedName(const std::string& path);

	bool ReadWholeFile(const std::filesystem::path& path, std::vector<uint8_t>& bytes);
	bool WriteFileSafely(const std::filesystem::path& path, const uint8_t* bytes, size_t size);
	bool GetFileState(const std::filesystem::path& path, uint64_t& size, int64_t& writeTime);

	// What the folder holds of one kind of file, and removing those
	void CountFiles(const std::string& extension, uint32_t& files, uint64_t& bytes);
	uint32_t RemoveFiles(const std::string& extension);
}
