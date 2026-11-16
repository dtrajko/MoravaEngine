#include "NavMeshFile.h"

#include "Assets/AssetFiles.h"

#include <cstring>
#include <filesystem>
#include <fstream>


namespace NavMeshFile
{
	static constexpr char Magic[4] = { 'M', 'N', 'A', 'V' };
	static constexpr uint32_t Version = 1;

	std::string GetPathForScene(const std::string& scenePath)
	{
		return std::filesystem::u8path(scenePath).replace_extension(Extension).u8string();
	}

	// ------------------------------------------------------------------------------------------------------------ writing

	template<typename T>
	static void Put(std::vector<uint8_t>& out, const T& value)
	{
		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
		out.insert(out.end(), bytes, bytes + sizeof(T));
	}

	static void PutSettings(std::vector<uint8_t>& out, const NavMeshSettings& s)
	{
		for (float value : { s.AgentRadius, s.AgentHeight, s.AgentClimb, s.MaxSlopeDegrees, s.CellSize, s.CellHeight, s.TileSize,
			s.RegionMinArea, s.EdgeMaxError, s.DetailSampleDistance, s.DetailSampleMaxError })
		{
			Put(out, value);
		}
	}

	bool Write(const std::string& path, const NavMeshData& data, std::string& error)
	{
		std::vector<uint8_t> out;
		out.insert(out.end(), Magic, Magic + 4);
		Put(out, Version);
		PutSettings(out, data.Settings);
		Put(out, data.SourceHash);
		Put(out, data.Origin.x);
		Put(out, data.Origin.y);
		Put(out, data.Origin.z);
		Put(out, data.TileWorldSize);
		Put(out, (int32_t)data.TilesX);
		Put(out, (int32_t)data.TilesZ);
		Put(out, (int32_t)data.MaxPolysPerTile);
		Put(out, (int32_t)data.Polygons);
		Put(out, (uint32_t)data.Tiles.size());
		for (const NavMeshTile& tile : data.Tiles)
		{
			Put(out, (int32_t)tile.X);
			Put(out, (int32_t)tile.Z);
			Put(out, (uint32_t)tile.Data.size());
			out.insert(out.end(), tile.Data.begin(), tile.Data.end());
		}

		const std::filesystem::path target = std::filesystem::u8path(path);
		std::filesystem::path temporary = target;
		temporary += ".tmp";
		{
			std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
			if (!file || !file.write(reinterpret_cast<const char*>(out.data()), (std::streamsize)out.size()))
			{
				error = "'" + path + "' can't be written";
				return false;
			}
		}
		std::error_code code;
		std::filesystem::rename(temporary, target, code);
		if (code)
		{
			std::filesystem::remove(temporary, code);
			error = "'" + path + "' can't be replaced (is it open elsewhere?)";
			return false;
		}
		return true;
	}

	// ------------------------------------------------------------------------------------------------------------ reading

	struct Reader
	{
		const std::vector<uint8_t>& Bytes;
		size_t Offset = 0;
		bool Failed = false;

		template<typename T>
		T Get()
		{
			T value {};
			if (Offset + sizeof(T) > Bytes.size())
			{
				Failed = true;
				return value;
			}
			std::memcpy(&value, Bytes.data() + Offset, sizeof(T));
			Offset += sizeof(T);
			return value;
		}
	};

	bool Read(const std::string& path, NavMeshData& data, std::string& error)
	{
		std::vector<uint8_t> bytes;
		if (!AssetFiles::Read(path, bytes))
		{
			error = "it can't be read";
			return false;
		}
		if (bytes.size() < 8 || std::memcmp(bytes.data(), Magic, 4) != 0)
		{
			error = "it isn't a navmesh file";
			return false;
		}
		Reader in { bytes, 4 };
		const uint32_t version = in.Get<uint32_t>();
		if (version != Version)
		{
			error = "version " + std::to_string(version) + " (this engine reads " + std::to_string(Version) + "): bake it again";
			return false;
		}
		NavMeshData result;
		NavMeshSettings& s = result.Settings;
		for (float* value : { &s.AgentRadius, &s.AgentHeight, &s.AgentClimb, &s.MaxSlopeDegrees, &s.CellSize, &s.CellHeight, &s.TileSize,
			&s.RegionMinArea, &s.EdgeMaxError, &s.DetailSampleDistance, &s.DetailSampleMaxError })
		{
			*value = in.Get<float>();
		}
		result.SourceHash = in.Get<uint64_t>();
		result.Origin.x = in.Get<float>();
		result.Origin.y = in.Get<float>();
		result.Origin.z = in.Get<float>();
		result.TileWorldSize = in.Get<float>();
		result.TilesX = in.Get<int32_t>();
		result.TilesZ = in.Get<int32_t>();
		result.MaxPolysPerTile = in.Get<int32_t>();
		result.Polygons = in.Get<int32_t>();
		const uint32_t tileCount = in.Get<uint32_t>();
		if (in.Failed || result.TilesX < 0 || result.TilesZ < 0 || (uint64_t)tileCount > (uint64_t)result.TilesX * (uint64_t)result.TilesZ)
		{
			error = "it is damaged (its header)";
			return false;
		}
		result.Tiles.resize(tileCount);
		for (NavMeshTile& tile : result.Tiles)
		{
			tile.X = in.Get<int32_t>();
			tile.Z = in.Get<int32_t>();
			const uint32_t size = in.Get<uint32_t>();
			if (in.Failed || in.Offset + size > bytes.size())
			{
				error = "it is damaged (cut short)";
				return false;
			}
			tile.Data.assign(bytes.begin() + in.Offset, bytes.begin() + in.Offset + size);
			in.Offset += size;
		}
		data = std::move(result);
		return true;
	}
}
