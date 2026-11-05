#pragma once

#include "Project/AssetRegistry.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * The pack file (docs/engine/Game_Engine_Runtime_Guide.html, 10.3): a game's assets in one file, Game.mpak.
 *
 *   header   'MPAK', version, where the table of contents is and its size, its hash, the number of entries
 *   data     each entry's bytes, LZ4-compressed where that saves something
 *   table    per entry: asset ID, offset, stored size, size, hash of the bytes, type, compression, kind, the path
 *
 * An entry is found by its path as saved files write it (ProjectPaths::ToStored), and an asset's ID leads to that path: the
 * engine's components and a game's scripts still name files. One file can have several entries of different kinds: an image
 * used in two ways is cooked twice. A damaged entry is refused when it is read: every entry has a hash.
 */
class AssetPack
{
public:
	enum class Kind : uint16_t
	{
		File = 0,  // the file's bytes as they are: scenes, materials, sounds, fonts, an image that isn't a cooked material map
		Model,     // a model file's cooked .mmesh
		ColorMap,  // an image's cooked .ktx2, by its use
		NormalMap,
		DataMap,
		Shader     // a .glsl file's SPIR-V, per stage
	};

	enum class Compression : uint16_t { None = 0, LZ4 = 1 };

	struct Entry
	{
		AssetID ID = 0;
		AssetType Type = AssetType::None;
		Kind EntryKind = Kind::File;
		Compression Stored = Compression::None;
		uint64_t Offset = 0;
		uint64_t StoredSize = 0;
		uint64_t Size = 0;
		uint64_t Hash = 0;
		std::string Path;
	};

	static constexpr uint32_t Version = 1;

	// Writes a pack to a stream, entry by entry: the bytes needn't all be in memory at once
	class Writer
	{
	public:
		explicit Writer(std::ostream& out);
		void Add(AssetID id, AssetType type, Kind kind, const std::string& path, const uint8_t* bytes, size_t size);
		// False: the stream failed. Nothing is complete before this
		bool Finish();

		uint64_t GetStoredBytes() const { return m_Offset; }
		uint64_t GetOriginalBytes() const { return m_OriginalBytes; }

	private:
		std::ostream& m_Out;
		std::vector<Entry> m_Entries;
		uint64_t m_Offset = 0;
		uint64_t m_OriginalBytes = 0;
	};

	// False (error says why): not a pack, another version, or a cut or damaged table
	bool Open(std::unique_ptr<std::istream> in, std::string& error);

	const Entry* Find(const std::string& path, Kind kind) const;
	bool Contains(const std::string& path) const; // of any kind
	const std::vector<Entry>& GetEntries() const { return m_Entries; }

	// Any thread. False (error says why): the entry can't be read or is damaged
	bool Read(const Entry& entry, std::vector<uint8_t>& bytes, std::string& error) const;

	// A pack made in memory is read back; a cut or damaged one is refused. Logs 'self-test failed'
	static bool SelfTest();

private:
	std::unique_ptr<std::istream> m_In;
	mutable std::mutex m_Mutex;
	std::vector<Entry> m_Entries;
	std::unordered_map<std::string, std::vector<size_t>> m_ByPath; // the entries of each file, one per kind
};
