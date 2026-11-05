#pragma once

#include "ModelData.h"

#include <cstdint>
#include <string>
#include <vector>


/**
 * Cooked models (docs/engine/Game_Engine_Runtime_Guide.html, 10.2). The first load of a model file imports it through assimp
 * and writes a .mmesh file into the cook cache: <Project>/Library/Cooked, or Resources/Cache/Cooked without a project. Later
 * loads read that file, as long as the source files (the model and what its import opened: an .obj's .mtl, a .gltf's .bin)
 * and the cooker's version are the same. The cache can be deleted at any time.
 */
namespace ModelCook
{
	struct LoadInfo
	{
		bool FromCache = false;
		double Milliseconds = 0.0;
	};

	// False: the file doesn't exist or can't be imported (error says why). A file with only animations is fine. With a pack
	// mounted the pack's cooked model is read, and nothing else
	bool Load(const std::string& path, ModelData& data, LoadInfo& info, std::string& error);

	// What turns a source file into plain data: the editor gives it (ModelImport, through assimp). A player has none: it reads
	// what a pack or the cache holds
	using Importer = bool(*)(const std::string& path, ModelData& data, std::vector<std::string>& files, std::string& error);
	void SetImporter(Importer importer);

	// The cache's file of a model, after a Load
	std::string GetCookedFile(const std::string& path);

	// For the Assets panel: this session's loads, and what the cache folder holds
	struct Stats
	{
		uint32_t Read = 0;
		double ReadMilliseconds = 0.0;
		uint32_t Imported = 0;
		double ImportMilliseconds = 0.0;
	};
	const Stats& GetStats();
	struct CacheContents
	{
		std::string Folder;
		uint32_t Files = 0;
		uint64_t Bytes = 0;
	};
	CacheContents GetCacheContents();
	uint32_t ClearCache(); // the number of files removed

	// A model made in code goes to bytes and back; a cut or damaged file is refused. Logs 'self-test failed'
	bool SelfTest();
}
