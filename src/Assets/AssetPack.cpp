#include "AssetPack.h"

#include "CookFiles.h"

#include "Core/Log.h"

#include "lz4.h"
#include "lz4hc.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <sstream>


namespace
{
	const char Magic[4] = { 'M', 'P', 'A', 'K' };

	struct Header
	{
		char FileMagic[4];
		uint32_t FileVersion;
		uint64_t TableOffset;
		uint64_t TableSize;
		uint64_t TableHash;
		uint32_t EntryCount;
		uint32_t Reserved;
	};
	static_assert(sizeof(Header) == 40, "the pack's header is part of its format");

	struct TableEntry
	{
		uint64_t ID;
		uint64_t Offset;
		uint64_t StoredSize;
		uint64_t Size;
		uint64_t Hash;
		uint16_t Type;
		uint16_t Stored;
		uint16_t EntryKind;
		uint16_t PathSize; // the path's bytes follow
	};
	static_assert(sizeof(TableEntry) == 48, "the pack's table is part of its format");

	constexpr size_t SmallestToCompress = 256;
	constexpr int CompressionLevel = 9;
}

AssetPack::Writer::Writer(std::ostream& out)
	: m_Out(out)
{
	const Header empty = {};
	m_Out.write(reinterpret_cast<const char*>(&empty), sizeof(empty));
	m_Offset = sizeof(Header);
}

void AssetPack::Writer::Add(AssetID id, AssetType type, Kind kind, const std::string& path, const uint8_t* bytes, size_t size)
{
	Entry entry;
	entry.ID = id;
	entry.Type = type;
	entry.EntryKind = kind;
	entry.Path = path;
	entry.Offset = m_Offset;
	entry.Size = size;
	entry.Hash = CookFiles::HashBytes(bytes, size);

	// Kept only when it saves something worth a decompression: block-compressed textures mostly don't
	std::vector<char> compressed;
	if (size >= SmallestToCompress && size < (size_t)LZ4_MAX_INPUT_SIZE)
	{
		compressed.resize((size_t)LZ4_compressBound((int)size));
		const int written = LZ4_compress_HC(reinterpret_cast<const char*>(bytes), compressed.data(), (int)size, (int)compressed.size(), CompressionLevel);
		if (written > 0 && (uint64_t)written < size - size / 20)
		{
			entry.Stored = Compression::LZ4;
			entry.StoredSize = (uint64_t)written;
			m_Out.write(compressed.data(), written);
		}
	}
	if (entry.Stored == Compression::None)
	{
		entry.StoredSize = size;
		m_Out.write(reinterpret_cast<const char*>(bytes), (std::streamsize)size);
	}
	m_Offset += entry.StoredSize;
	m_OriginalBytes += size;
	m_Entries.push_back(std::move(entry));
}

bool AssetPack::Writer::Finish()
{
	std::string table;
	for (const Entry& entry : m_Entries)
	{
		const TableEntry row = { entry.ID, entry.Offset, entry.StoredSize, entry.Size, entry.Hash, (uint16_t)entry.Type, (uint16_t)entry.Stored,
			(uint16_t)entry.EntryKind, (uint16_t)entry.Path.size() };
		table.append(reinterpret_cast<const char*>(&row), sizeof(row));
		table.append(entry.Path);
	}
	m_Out.write(table.data(), (std::streamsize)table.size());

	Header header = {};
	memcpy(header.FileMagic, Magic, sizeof(Magic));
	header.FileVersion = AssetPack::Version;
	header.TableOffset = m_Offset;
	header.TableSize = table.size();
	header.TableHash = CookFiles::HashBytes(reinterpret_cast<const uint8_t*>(table.data()), table.size());
	header.EntryCount = (uint32_t)m_Entries.size();
	m_Out.seekp(0);
	m_Out.write(reinterpret_cast<const char*>(&header), sizeof(header));
	m_Out.flush();
	return m_Out.good();
}

bool AssetPack::Open(std::unique_ptr<std::istream> in, std::string& error)
{
	m_Entries.clear();
	m_ByPath.clear();

	in->seekg(0, std::ios::end);
	const uint64_t fileSize = (uint64_t)in->tellg();
	in->seekg(0);
	Header header = {};
	in->read(reinterpret_cast<char*>(&header), sizeof(header));
	if (!*in || memcmp(header.FileMagic, Magic, sizeof(Magic)) != 0)
	{
		error = "it is not a pack file";
		return false;
	}
	if (header.FileVersion != Version)
	{
		error = "it is a pack of version " + std::to_string(header.FileVersion) + ", this engine reads version " + std::to_string(Version);
		return false;
	}
	if (header.TableOffset > fileSize || header.TableSize > fileSize - header.TableOffset)
	{
		error = "the file is cut short";
		return false;
	}

	std::vector<uint8_t> table((size_t)header.TableSize);
	in->seekg((std::streamoff)header.TableOffset);
	in->read(reinterpret_cast<char*>(table.data()), (std::streamsize)table.size());
	if (!*in || CookFiles::HashBytes(table.data(), table.size()) != header.TableHash)
	{
		error = "its table of contents is damaged";
		return false;
	}

	size_t position = 0;
	m_Entries.reserve(header.EntryCount);
	for (uint32_t i = 0; i < header.EntryCount; i++)
	{
		TableEntry row;
		if (table.size() - position < sizeof(row))
		{
			error = "its table of contents is damaged";
			return false;
		}
		memcpy(&row, table.data() + position, sizeof(row));
		position += sizeof(row);
		if (table.size() - position < row.PathSize || row.Offset > header.TableOffset || row.StoredSize > header.TableOffset - row.Offset)
		{
			error = "its table of contents is damaged";
			return false;
		}
		Entry entry;
		entry.ID = row.ID;
		entry.Type = (AssetType)row.Type;
		entry.Stored = (Compression)row.Stored;
		entry.EntryKind = (Kind)row.EntryKind;
		entry.Offset = row.Offset;
		entry.StoredSize = row.StoredSize;
		entry.Size = row.Size;
		entry.Hash = row.Hash;
		entry.Path.assign(reinterpret_cast<const char*>(table.data() + position), row.PathSize);
		position += row.PathSize;
		m_ByPath[entry.Path].push_back(m_Entries.size());
		m_Entries.push_back(std::move(entry));
	}
	m_In = std::move(in);
	return true;
}

const AssetPack::Entry* AssetPack::Find(const std::string& path, Kind kind) const
{
	const auto found = m_ByPath.find(path);
	if (found != m_ByPath.end())
	{
		for (const size_t index : found->second)
		{
			if (m_Entries[index].EntryKind == kind)
			{
				return &m_Entries[index];
			}
		}
	}
	return nullptr;
}

bool AssetPack::Contains(const std::string& path) const
{
	return m_ByPath.find(path) != m_ByPath.end();
}

bool AssetPack::Read(const Entry& entry, std::vector<uint8_t>& bytes, std::string& error) const
{
	std::vector<char> stored((size_t)entry.StoredSize);
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_In->clear();
		m_In->seekg((std::streamoff)entry.Offset);
		m_In->read(stored.data(), (std::streamsize)stored.size());
		if (!*m_In)
		{
			error = "the pack file can't be read there";
			return false;
		}
	}

	if (entry.Stored == Compression::LZ4)
	{
		if (entry.Size > (uint64_t)INT_MAX || entry.StoredSize > (uint64_t)INT_MAX)
		{
			error = "its entry in the pack is damaged";
			return false;
		}
		bytes.resize((size_t)entry.Size);
		const int written = LZ4_decompress_safe(stored.data(), reinterpret_cast<char*>(bytes.data()), (int)stored.size(), (int)bytes.size());
		if (written < 0 || (uint64_t)written != entry.Size)
		{
			error = "its entry in the pack is damaged";
			return false;
		}
	}
	else
	{
		bytes.assign(stored.begin(), stored.end());
	}
	if (CookFiles::HashBytes(bytes.data(), bytes.size()) != entry.Hash)
	{
		error = "its entry in the pack is damaged";
		return false;
	}
	return true;
}

bool AssetPack::SelfTest()
{
	auto fail = [](const char* what)
	{
		Log::GetLogger()->error("Asset pack self-test failed: {0}", what);
		return false;
	};

	std::vector<uint8_t> repeating(4096, 7);
	std::vector<uint8_t> varied(1024);
	uint32_t value = 1;
	for (uint8_t& byte : varied)
	{
		value = value * 1664525u + 1013904223u;
		byte = (uint8_t)(value >> 24);
	}

	std::ostringstream out(std::ios::binary);
	AssetPack::Writer writer(out);
	writer.Add(11, AssetType::Scene, Kind::File, "Scenes/A.mscene", repeating.data(), repeating.size());
	writer.Add(22, AssetType::Texture, Kind::ColorMap, "Textures/B.png", varied.data(), varied.size());
	writer.Add(22, AssetType::Texture, Kind::DataMap, "Textures/B.png", repeating.data(), 10);
	if (!writer.Finish())
	{
		return fail("the pack could not be written");
	}
	const std::string file = out.str();

	std::string error;
	AssetPack pack;
	if (!pack.Open(std::make_unique<std::istringstream>(file, std::ios::binary), error) || pack.GetEntries().size() != 3)
	{
		return fail("the pack could not be opened");
	}
	const Entry* scene = pack.Find("Scenes/A.mscene", Kind::File);
	const Entry* color = pack.Find("Textures/B.png", Kind::ColorMap);
	const Entry* data = pack.Find("Textures/B.png", Kind::DataMap);
	if (!scene || !color || !data || pack.Find("Textures/B.png", Kind::NormalMap) || pack.Find("Textures/C.png", Kind::File) || !pack.Contains("Textures/B.png"))
	{
		return fail("an entry isn't found by its path and kind");
	}
	if (scene->Stored != Compression::LZ4 || color->Stored != Compression::None || scene->ID != 11 || color->Type != AssetType::Texture)
	{
		return fail("an entry isn't stored as it should be");
	}
	std::vector<uint8_t> bytes;
	if (!pack.Read(*scene, bytes, error) || bytes != repeating)
	{
		return fail("a compressed entry reads differently");
	}
	if (!pack.Read(*color, bytes, error) || bytes != varied)
	{
		return fail("an entry reads differently");
	}

	// A damaged entry, a damaged table, a cut file and something else altogether
	std::string damaged = file;
	damaged[(size_t)color->Offset + 3] ^= 0x40;
	AssetPack damagedPack;
	if (!damagedPack.Open(std::make_unique<std::istringstream>(damaged, std::ios::binary), error))
	{
		return fail("a damaged entry fails the whole pack");
	}
	if (damagedPack.Read(*damagedPack.Find("Textures/B.png", Kind::ColorMap), bytes, error))
	{
		return fail("a damaged entry is read");
	}
	damaged = file;
	damaged[damaged.size() - 2] ^= 0x40;
	if (AssetPack().Open(std::make_unique<std::istringstream>(damaged, std::ios::binary), error))
	{
		return fail("a damaged table is accepted");
	}
	if (AssetPack().Open(std::make_unique<std::istringstream>(file.substr(0, file.size() / 2), std::ios::binary), error))
	{
		return fail("a cut file is accepted");
	}
	if (AssetPack().Open(std::make_unique<std::istringstream>(std::string(100, 'x'), std::ios::binary), error))
	{
		return fail("something other than a pack is accepted");
	}
	return true;
}
