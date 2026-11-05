#include "ModelCook.h"
#include "Assets/AssetFiles.h"

#include "CookFiles.h"

#include "Core/Log.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <type_traits>


namespace fs = std::filesystem;

namespace ModelCook
{
	using CookFiles::GetFileState;
	using CookFiles::HashBytes;
	using CookFiles::ReadWholeFile;

	static const char Magic[4] = { 'M', 'M', 'S', 'H' };
	// Raise it when the file's layout, the import's flags or what the import does changes: older files are cooked again
	static constexpr uint32_t CookerVersion = 1;

	static Stats s_Stats;

	struct SourceFile
	{
		std::string Path; // relative to the model's folder: the model and its files can move together
		uint64_t Size = 0;
		int64_t WriteTime = 0;
		uint64_t Hash = 0;
	};

	// ---- Bytes

	class Writer
	{
	public:
		std::vector<uint8_t> Bytes;

		void Raw(const void* data, size_t size)
		{
			if (size > 0)
			{
				const uint8_t* start = static_cast<const uint8_t*>(data);
				Bytes.insert(Bytes.end(), start, start + size);
			}
		}

		template<typename T>
		void Value(const T& value)
		{
			static_assert(std::is_trivially_copyable<T>::value, "written as bytes");
			Raw(&value, sizeof(T));
		}

		void String(const std::string& text)
		{
			Value((uint32_t)text.size());
			Raw(text.data(), text.size());
		}

		template<typename T>
		void Array(const std::vector<T>& items)
		{
			static_assert(std::is_trivially_copyable<T>::value, "written as bytes");
			Value((uint64_t)items.size());
			Raw(items.data(), items.size() * sizeof(T));
		}
	};

	// Every read is checked against the end: a cut file reads as false, never past the end
	class Reader
	{
	public:
		Reader(const uint8_t* data, size_t size) : m_Data(data), m_Size(size) {}

		bool Raw(void* out, size_t size)
		{
			if (size > m_Size - m_Position)
			{
				return false;
			}
			if (size > 0)
			{
				memcpy(out, m_Data + m_Position, size);
				m_Position += size;
			}
			return true;
		}

		template<typename T>
		bool Value(T& value)
		{
			return Raw(&value, sizeof(T));
		}

		bool String(std::string& text)
		{
			uint32_t size = 0;
			if (!Value(size) || size > m_Size - m_Position)
			{
				return false;
			}
			text.resize(size);
			return size == 0 || Raw(&text[0], size);
		}

		template<typename T>
		bool Array(std::vector<T>& items)
		{
			uint64_t count = 0;
			if (!Value(count) || count > (m_Size - m_Position) / sizeof(T))
			{
				return false;
			}
			items.resize((size_t)count);
			return Raw(items.data(), (size_t)count * sizeof(T));
		}

		const uint8_t* GetCurrent() const { return m_Data + m_Position; }
		size_t GetRemaining() const { return m_Size - m_Position; }

	private:
		const uint8_t* m_Data;
		size_t m_Size;
		size_t m_Position = 0;
	};

	// ---- The model's data

	static void WriteMaterial(Writer& out, const ModelMaterialData& material)
	{
		out.String(material.Name);
		out.Value((uint8_t)material.HasDiffuseColor);
		out.Value(material.DiffuseColor);
		out.Value((uint8_t)material.HasEmissiveColor);
		out.Value(material.EmissiveColor);
		out.Value((uint8_t)material.HasShininess);
		out.Value(material.Shininess);
		out.Value((uint8_t)material.HasReflectivity);
		out.Value(material.Reflectivity);
		out.Value((uint8_t)material.HasMetallicFactor);
		out.Value(material.MetallicFactor);
		out.Value((uint8_t)material.HasRoughnessFactor);
		out.Value(material.RoughnessFactor);
		for (const std::string* map : { &material.AlbedoMap, &material.NormalMap, &material.ShininessMap, &material.DiffuseRoughnessMap,
			&material.ReflectionFactorMap, &material.MetalnessMap, &material.EmissiveMap, &material.AmbientOcclusionMap })
		{
			out.String(*map);
		}
	}

	static bool ReadFlag(Reader& in, bool& flag)
	{
		uint8_t value = 0;
		if (!in.Value(value))
		{
			return false;
		}
		flag = value != 0;
		return true;
	}

	static bool ReadMaterial(Reader& in, ModelMaterialData& material)
	{
		if (!in.String(material.Name) ||
			!ReadFlag(in, material.HasDiffuseColor) || !in.Value(material.DiffuseColor) ||
			!ReadFlag(in, material.HasEmissiveColor) || !in.Value(material.EmissiveColor) ||
			!ReadFlag(in, material.HasShininess) || !in.Value(material.Shininess) ||
			!ReadFlag(in, material.HasReflectivity) || !in.Value(material.Reflectivity) ||
			!ReadFlag(in, material.HasMetallicFactor) || !in.Value(material.MetallicFactor) ||
			!ReadFlag(in, material.HasRoughnessFactor) || !in.Value(material.RoughnessFactor))
		{
			return false;
		}
		for (std::string* map : { &material.AlbedoMap, &material.NormalMap, &material.ShininessMap, &material.DiffuseRoughnessMap,
			&material.ReflectionFactorMap, &material.MetalnessMap, &material.EmissiveMap, &material.AmbientOcclusionMap })
		{
			if (!in.String(*map))
			{
				return false;
			}
		}
		return true;
	}

	static void WritePayload(Writer& out, const ModelData& data)
	{
		out.Value((uint8_t)data.IsAnimated);
		out.Array(data.StaticVertices);
		out.Array(data.AnimatedVertices);
		out.Array(data.Indices);

		out.Value((uint32_t)data.Meshes.size());
		for (const ModelMeshData& mesh : data.Meshes)
		{
			out.String(mesh.Name);
			out.Value(mesh.BaseVertex);
			out.Value(mesh.BaseIndex);
			out.Value(mesh.IndexCount);
			out.Value(mesh.VertexCount);
			out.Value(mesh.TriangleCount);
			out.Value(mesh.MaterialIndex);
			out.Value((uint8_t)mesh.IsRigged);
			out.Value(mesh.BoundsMin);
			out.Value(mesh.BoundsMax);
		}

		out.Value((uint32_t)data.Materials.size());
		for (const ModelMaterialData& material : data.Materials)
		{
			WriteMaterial(out, material);
		}

		out.Value((uint32_t)data.Nodes.size());
		for (size_t n = 0; n < data.Nodes.size(); n++)
		{
			out.String(data.Nodes[n].Name);
			out.Value((int32_t)data.Nodes[n].Parent);
			out.Value(data.Nodes[n].Matrix);
			out.Array(data.NodeMeshes[n]);
		}

		out.Value((uint32_t)data.BoneNames.size());
		for (const std::string& name : data.BoneNames)
		{
			out.String(name);
		}
		out.Array(data.BoneOffsets);

		out.Value((uint32_t)data.Clips.size());
		for (const AnimationClip& clip : data.Clips)
		{
			out.String(clip.GetName());
			out.Value(clip.GetDuration());
			out.Value(clip.GetTicksPerSecond());
			out.Value((uint32_t)clip.GetChannels().size());
			for (const AnimationClip::Channel& channel : clip.GetChannels())
			{
				out.String(channel.Node);
				out.Array(channel.PositionTimes);
				out.Array(channel.Positions);
				out.Array(channel.RotationTimes);
				out.Array(channel.Rotations);
				out.Array(channel.ScaleTimes);
				out.Array(channel.Scales);
			}
		}
	}

	static bool ReadPayload(Reader& in, ModelData& data)
	{
		uint32_t count = 0;
		if (!ReadFlag(in, data.IsAnimated) || !in.Array(data.StaticVertices) || !in.Array(data.AnimatedVertices) || !in.Array(data.Indices) || !in.Value(count))
		{
			return false;
		}

		data.Meshes.resize(count);
		for (ModelMeshData& mesh : data.Meshes)
		{
			if (!in.String(mesh.Name) || !in.Value(mesh.BaseVertex) || !in.Value(mesh.BaseIndex) || !in.Value(mesh.IndexCount) || !in.Value(mesh.VertexCount) ||
				!in.Value(mesh.TriangleCount) || !in.Value(mesh.MaterialIndex) || !ReadFlag(in, mesh.IsRigged) || !in.Value(mesh.BoundsMin) || !in.Value(mesh.BoundsMax))
			{
				return false;
			}
		}

		if (!in.Value(count))
		{
			return false;
		}
		data.Materials.resize(count);
		for (ModelMaterialData& material : data.Materials)
		{
			if (!ReadMaterial(in, material))
			{
				return false;
			}
		}

		if (!in.Value(count))
		{
			return false;
		}
		data.Nodes.resize(count);
		data.NodeMeshes.resize(count);
		for (size_t n = 0; n < data.Nodes.size(); n++)
		{
			int32_t parent = -1;
			if (!in.String(data.Nodes[n].Name) || !in.Value(parent) || !in.Value(data.Nodes[n].Matrix) || !in.Array(data.NodeMeshes[n]))
			{
				return false;
			}
			data.Nodes[n].Parent = parent;
		}

		if (!in.Value(count))
		{
			return false;
		}
		data.BoneNames.resize(count);
		for (std::string& name : data.BoneNames)
		{
			if (!in.String(name))
			{
				return false;
			}
		}
		if (!in.Array(data.BoneOffsets) || !in.Value(count))
		{
			return false;
		}

		data.Clips.resize(count);
		for (AnimationClip& clip : data.Clips)
		{
			std::string name;
			float duration = 0.0f, ticksPerSecond = 0.0f;
			uint32_t channels = 0;
			if (!in.String(name) || !in.Value(duration) || !in.Value(ticksPerSecond) || !in.Value(channels))
			{
				return false;
			}
			clip.SetName(name);
			clip.SetDuration(duration);
			clip.SetTicksPerSecond(ticksPerSecond);
			clip.GetChannels().resize(channels);
			for (AnimationClip::Channel& channel : clip.GetChannels())
			{
				if (!in.String(channel.Node) || !in.Array(channel.PositionTimes) || !in.Array(channel.Positions) || !in.Array(channel.RotationTimes) ||
					!in.Array(channel.Rotations) || !in.Array(channel.ScaleTimes) || !in.Array(channel.Scales))
				{
					return false;
				}
			}
		}
		return true;
	}

	// ---- The file: what it was made from, then the data and the hash that tells it is whole

	static std::vector<uint8_t> MakeFile(const ModelData& data, const std::vector<SourceFile>& sources)
	{
		Writer payload;
		WritePayload(payload, data);

		Writer out;
		out.Bytes.reserve(payload.Bytes.size() + 256);
		out.Raw(Magic, sizeof(Magic));
		out.Value(CookerVersion);
		out.Value((uint32_t)sizeof(H2M::VertexH2M));
		out.Value((uint32_t)sizeof(H2M::AnimatedVertex));
		out.Value((uint32_t)sources.size());
		for (const SourceFile& source : sources)
		{
			out.String(source.Path);
			out.Value(source.Size);
			out.Value(source.WriteTime);
			out.Value(source.Hash);
		}
		out.Value((uint64_t)payload.Bytes.size());
		out.Value(HashBytes(payload.Bytes.data(), payload.Bytes.size()));
		out.Raw(payload.Bytes.data(), payload.Bytes.size());
		return std::move(out.Bytes);
	}

	// False: not a file of this cooker
	static bool ReadHeader(Reader& in, std::vector<SourceFile>& sources)
	{
		char magic[4] = {};
		uint32_t version = 0, staticVertexSize = 0, animatedVertexSize = 0, count = 0;
		if (!in.Raw(magic, sizeof(magic)) || memcmp(magic, Magic, sizeof(Magic)) != 0 || !in.Value(version) || version != CookerVersion ||
			!in.Value(staticVertexSize) || staticVertexSize != sizeof(H2M::VertexH2M) ||
			!in.Value(animatedVertexSize) || animatedVertexSize != sizeof(H2M::AnimatedVertex) || !in.Value(count) || count > in.GetRemaining())
		{
			return false;
		}
		sources.resize(count);
		for (SourceFile& source : sources)
		{
			if (!in.String(source.Path) || !in.Value(source.Size) || !in.Value(source.WriteTime) || !in.Value(source.Hash))
			{
				return false;
			}
		}
		return true;
	}

	static bool ReadData(Reader& in, ModelData& data)
	{
		uint64_t size = 0, hash = 0;
		if (!in.Value(size) || !in.Value(hash) || size != in.GetRemaining() || HashBytes(in.GetCurrent(), (size_t)size) != hash)
		{
			return false;
		}
		return ReadPayload(in, data);
	}

	// ---- Files

	static bool DescribeSource(const fs::path& file, const fs::path& folder, SourceFile& source)
	{
		std::vector<uint8_t> bytes;
		if (!GetFileState(file, source.Size, source.WriteTime) || !ReadWholeFile(file, bytes))
		{
			return false;
		}
		source.Hash = HashBytes(bytes.data(), bytes.size());
		source.Path = file.lexically_relative(folder).generic_u8string();
		return !source.Path.empty();
	}

	// Whether the sources are as they were cooked. By size and write time, else by content (a Git checkout gives an unchanged
	// file a new time): then touched is set, and the times are brought up to date for the cooked file to be written again
	static bool SourcesUnchanged(std::vector<SourceFile>& sources, const fs::path& folder, bool& touched)
	{
		for (SourceFile& source : sources)
		{
			const fs::path file = folder / fs::u8path(source.Path);
			uint64_t size = 0;
			int64_t writeTime = 0;
			if (!GetFileState(file, size, writeTime) || size != source.Size)
			{
				return false;
			}
			if (writeTime != source.WriteTime)
			{
				std::vector<uint8_t> bytes;
				if (!ReadWholeFile(file, bytes) || HashBytes(bytes.data(), bytes.size()) != source.Hash)
				{
					return false;
				}
				source.WriteTime = writeTime;
				touched = true;
			}
		}
		return true;
	}

	static void WriteCookedFile(const fs::path& path, const ModelData& data, const std::vector<SourceFile>& sources)
	{
		const std::vector<uint8_t> bytes = MakeFile(data, sources);
		if (!CookFiles::WriteFileSafely(path, bytes.data(), bytes.size()))
		{
			Log::GetLogger()->warn("Cooked model '{0}' could not be written", path.generic_u8string());
		}
	}

	static bool ReadCookedFile(const fs::path& cooked, const fs::path& folder, ModelData& data)
	{
		std::vector<uint8_t> bytes;
		if (!ReadWholeFile(cooked, bytes))
		{
			return false;
		}
		Reader in(bytes.data(), bytes.size());
		std::vector<SourceFile> sources;
		bool touched = false;
		if (!ReadHeader(in, sources) || !SourcesUnchanged(sources, folder, touched) || !ReadData(in, data))
		{
			return false;
		}
		if (touched)
		{
			WriteCookedFile(cooked, data, sources);
		}
		return true;
	}

	// ---- Loading

	static Importer s_Importer = nullptr;

	void SetImporter(Importer importer)
	{
		s_Importer = importer;
	}

	std::string GetCookedFile(const std::string& path)
	{
		return (fs::u8path(CookFiles::GetCacheFolder()) / fs::u8path(CookFiles::GetCookedName(path) + ".mmesh")).generic_u8string();
	}

	bool Load(const std::string& path, ModelData& data, LoadInfo& info, std::string& error)
	{
		const auto start = std::chrono::steady_clock::now();
		auto elapsed = [&]() { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count(); };

		if (AssetFiles::IsPackMounted())
		{
			std::vector<uint8_t> bytes;
			if (!AssetFiles::ReadCooked(path, AssetPack::Kind::Model, bytes))
			{
				error = "it isn't in the pack";
				return false;
			}
			Reader in(bytes.data(), bytes.size());
			std::vector<SourceFile> sources;
			data = ModelData();
			if (!ReadHeader(in, sources) || !ReadData(in, data))
			{
				error = "its entry in the pack is of an older cooker or damaged: build the pack again";
				return false;
			}
			info.FromCache = true;
			info.Milliseconds = elapsed();
			s_Stats.Read++;
			s_Stats.ReadMilliseconds += info.Milliseconds;
			return true;
		}

		std::error_code fileError;
		const fs::path file = fs::u8path(path);
		if (!fs::is_regular_file(file, fileError))
		{
			error = "the file doesn't exist";
			return false;
		}
		const fs::path model = fs::absolute(file, fileError).lexically_normal();
		const fs::path folder = model.parent_path();

		const fs::path cooked = fs::u8path(CookFiles::GetCacheFolder()) / fs::u8path(CookFiles::GetCookedName(path) + ".mmesh");
		data = ModelData();
		if (ReadCookedFile(cooked, folder, data))
		{
			info.FromCache = true;
			info.Milliseconds = elapsed();
			s_Stats.Read++;
			s_Stats.ReadMilliseconds += info.Milliseconds;
			return true;
		}

		if (!s_Importer)
		{
			error = "it isn't cooked (or changed since), and a player doesn't import models: build a pack, or open the project in the editor";
			return false;
		}
		std::vector<std::string> opened;
		if (!s_Importer(path, data, opened, error))
		{
			return false;
		}

		// What it was made from, each file once and the model first
		std::vector<fs::path> files = { model };
		for (const std::string& name : opened)
		{
			fs::path other = fs::absolute(fs::u8path(name), fileError).lexically_normal();
			if (std::find(files.begin(), files.end(), other) == files.end())
			{
				files.push_back(std::move(other));
			}
		}
		std::vector<SourceFile> sources(files.size());
		bool described = true;
		for (size_t i = 0; i < files.size(); i++)
		{
			described = described && DescribeSource(files[i], folder, sources[i]);
		}
		if (described)
		{
			WriteCookedFile(cooked, data, sources);
		}

		info.FromCache = false;
		info.Milliseconds = elapsed();
		s_Stats.Imported++;
		s_Stats.ImportMilliseconds += info.Milliseconds;
		return true;
	}

	const Stats& GetStats()
	{
		return s_Stats;
	}

	CacheContents GetCacheContents()
	{
		CacheContents contents;
		contents.Folder = CookFiles::GetCacheFolder();
		CookFiles::CountFiles(".mmesh", contents.Files, contents.Bytes);
		return contents;
	}

	uint32_t ClearCache()
	{
		return CookFiles::RemoveFiles(".mmesh");
	}

	// ---- Self-test

	bool SelfTest()
	{
		auto fail = [](const char* what)
		{
			Log::GetLogger()->error("Cooked model self-test failed: {0}", what);
			return false;
		};

		ModelData data;
		data.IsAnimated = true;
		H2M::AnimatedVertex vertex;
		vertex.Position = glm::vec3(1.0f, 2.0f, 3.0f);
		vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
		vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
		vertex.Binormal = glm::vec3(0.0f, 0.0f, 1.0f);
		vertex.Texcoord = glm::vec2(0.5f, 0.25f);
		vertex.AddBoneData(1, 0.75f);
		data.AnimatedVertices = { vertex, vertex, vertex };
		data.Indices = { { 0, 1, 2 } };
		ModelMeshData mesh;
		mesh.Name = "Mesh";
		mesh.VertexCount = 3;
		mesh.IndexCount = 3;
		mesh.TriangleCount = 1;
		mesh.IsRigged = true;
		mesh.BoundsMax = glm::vec3(4.0f);
		data.Meshes = { mesh };
		ModelMaterialData material;
		material.Name = "Material";
		material.HasDiffuseColor = true;
		material.DiffuseColor = glm::vec3(0.25f);
		material.NormalMap = "maps/normal.png";
		data.Materials = { material };
		data.Nodes = { { "Root", -1, glm::mat4(2.0f) }, { "Bone", 0, glm::mat4(1.0f) } };
		data.NodeMeshes = { { 0 }, {} };
		data.BoneNames = { "Bone" };
		data.BoneOffsets = { glm::mat4(3.0f) };
		AnimationClip clip;
		clip.SetName("Walk");
		clip.SetDuration(2.0f);
		clip.SetTicksPerSecond(30.0f);
		AnimationClip::Channel channel;
		channel.Node = "Bone";
		channel.RotationTimes = { 0.5f };
		channel.Rotations = { glm::quat(0.0f, 1.0f, 0.0f, 0.0f) };
		clip.GetChannels() = { channel };
		data.Clips = { clip };

		SourceFile source;
		source.Path = "Model.obj";
		source.Size = 7;
		source.WriteTime = 8;
		source.Hash = 9;
		const std::vector<uint8_t> bytes = MakeFile(data, { source });

		Reader in(bytes.data(), bytes.size());
		std::vector<SourceFile> sources;
		ModelData read;
		if (!ReadHeader(in, sources) || !ReadData(in, read))
		{
			return fail("a whole file is read");
		}
		if (sources.size() != 1 || sources[0].Path != "Model.obj" || sources[0].Hash != 9)
		{
			return fail("the sources");
		}
		if (!read.IsAnimated || read.AnimatedVertices.size() != 3 || read.AnimatedVertices[2].Texcoord != vertex.Texcoord || read.AnimatedVertices[2].IDs[0] != 1 ||
			read.Indices.size() != 1 || read.Indices[0].V3 != 2)
		{
			return fail("the vertices and indices");
		}
		if (read.Meshes.size() != 1 || read.Meshes[0].Name != "Mesh" || read.Meshes[0].TriangleCount != 1 || !read.Meshes[0].IsRigged || read.Meshes[0].BoundsMax != glm::vec3(4.0f))
		{
			return fail("the meshes");
		}
		if (read.Materials.size() != 1 || !read.Materials[0].HasDiffuseColor || read.Materials[0].HasShininess || read.Materials[0].DiffuseColor != glm::vec3(0.25f) ||
			read.Materials[0].NormalMap != "maps/normal.png" || !read.Materials[0].AlbedoMap.empty())
		{
			return fail("the materials");
		}
		if (read.Nodes.size() != 2 || read.Nodes[1].Parent != 0 || read.Nodes[0].Matrix != glm::mat4(2.0f) || read.NodeMeshes[0].size() != 1 || !read.NodeMeshes[1].empty() ||
			read.BoneNames.size() != 1 || read.BoneOffsets[0] != glm::mat4(3.0f))
		{
			return fail("the nodes and bones");
		}
		if (read.Clips.size() != 1 || read.Clips[0].GetName() != "Walk" || read.Clips[0].GetTicksPerSecond() != 30.0f ||
			read.Clips[0].GetChannels().size() != 1 || read.Clips[0].GetChannels()[0].Rotations[0] != channel.Rotations[0])
		{
			return fail("the clips");
		}

		// Cut anywhere, it isn't read; nor with one bit of its data changed
		for (size_t size = 0; size < bytes.size(); size += 13)
		{
			Reader cut(bytes.data(), size);
			std::vector<SourceFile> cutSources;
			ModelData cutData;
			if (ReadHeader(cut, cutSources) && ReadData(cut, cutData))
			{
				return fail("a cut file is refused");
			}
		}
		std::vector<uint8_t> damaged = bytes;
		damaged[damaged.size() - 5] ^= 0x10;
		Reader damagedReader(damaged.data(), damaged.size());
		std::vector<SourceFile> damagedSources;
		ModelData damagedData;
		if (ReadHeader(damagedReader, damagedSources) && ReadData(damagedReader, damagedData))
		{
			return fail("a damaged file is refused");
		}
		return true;
	}
}
