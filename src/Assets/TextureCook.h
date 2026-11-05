#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


/**
 * Cooked material maps (docs/engine/Game_Engine_Runtime_Guide.html, 10.2). An image used as a material map is cooked into a
 * .ktx2 file in the cook cache: a full mip chain, compressed to BC7 (color and data maps) or BC5 (normal maps). Cooking takes
 * seconds per image, so it runs on a thread of its own, at a lower priority: a map not cooked yet loads from its source file
 * as before, and the next load reads the cooked file. A map's use decides its format, so one image used in two ways has two
 * cooked files. The source file's sidecar can turn compression off (Import: Compression: None).
 */
namespace TextureCook
{
	enum class MapKind : uint32_t { Color = 0, Normal = 1, Data = 2 };
	enum class BlockFormat : uint32_t { BC7, BC7_SRGB, BC5 };

	struct CookedLevel
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint64_t Offset = 0; // into Bytes
		uint64_t Size = 0;
	};

	struct CookedTexture
	{
		BlockFormat Format = BlockFormat::BC7;
		uint32_t Width = 0;
		uint32_t Height = 0;
		std::vector<CookedLevel> Levels; // the largest first
		std::vector<uint8_t> Bytes;
		// Of the source's pixels: what a material needs without the pixels (the ray tracing average), linear for a color map
		glm::vec4 Average = glm::vec4(1.0f);
	};

	// The cooked file of a map, if there is one made from the source as it is now. False: load the source (and Request a cook)
	bool Read(const std::string& path, MapKind kind, CookedTexture& texture, double& milliseconds);
	// Queues the map for cooking, once; ignored when its sidecar turns compression off
	void Request(const std::string& path, MapKind kind);
	// Whether an image of this type can be cooked at all
	bool IsCookable(const std::string& path);
	// Cooks now, on all the cores, unless the cache's file is current (cooked: false). False: error says why. For a pack
	bool CookNow(const std::string& path, MapKind kind, bool& cooked, std::string& error);
	std::string GetCookedFile(const std::string& path, MapKind kind);
	// A pack's cooked map as pixels: for a GPU without these formats, as a pack holds no source images
	bool ReadPixels(const std::string& path, MapKind kind, std::vector<uint8_t>& rgba, uint32_t& width, uint32_t& height);
	// Stops at the next row of blocks: an unfinished file is never written. Before the program ends
	void Shutdown();

	// The source's sidecar: Import: Compression: None. Read when a map loads, so a change shows the next time it loads
	bool IsCompressionOff(const std::string& path);
	void SetCompressionOff(const std::string& path, bool off);

	// For the Assets panel: this session's loads and cooks, and what the cache folder holds
	struct Stats
	{
		uint32_t Read = 0;
		double ReadMilliseconds = 0.0;
		uint32_t FromSource = 0;
		double SourceMilliseconds = 0.0;
		uint32_t Cooked = 0;
		double CookSeconds = 0.0;
		uint32_t Failed = 0;
		uint32_t Waiting = 0;  // in the queue, with the one being cooked
		std::string Cooking;   // the one being cooked
	};
	Stats GetStats();
	void AddSourceLoad(double milliseconds);
	void CountCachedFiles(uint32_t& files, uint64_t& bytes);
	uint32_t ClearCache(); // the number of files removed

	// An image made in code is cooked and decoded again, in each format; mips are checked; a cut or damaged file is refused.
	// Logs 'self-test failed'
	bool SelfTest();
}
