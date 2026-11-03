#include "AssetRegistry.h"

#include "Project.h"

#include "Core/Log.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <set>
#include <unordered_map>

namespace fs = std::filesystem;


const char* GetAssetTypeName(AssetType type)
{
	switch (type)
	{
		case AssetType::Model:          return "Model";
		case AssetType::Texture:        return "Texture";
		case AssetType::EnvironmentMap: return "EnvironmentMap";
		case AssetType::Sound:          return "Sound";
		case AssetType::Font:           return "Font";
		case AssetType::Prefab:         return "Prefab";
		case AssetType::Material:       return "Material";
		case AssetType::Animator:       return "Animator";
		case AssetType::RenderTexture:  return "RenderTexture";
		case AssetType::Scene:          return "Scene";
		default:                        return "None";
	}
}

namespace AssetRegistry
{
	static const char* const SidecarExtension = ".meta";

	struct CachedID
	{
		fs::file_time_type WriteTime;
		AssetID ID = 0;
	};

	static bool s_Writable = false;
	static bool s_Scanned = false;
	static std::chrono::steady_clock::time_point s_LastScan;
	static double s_LastScanMilliseconds = 0.0;
	static std::vector<Record> s_Records;
	static std::unordered_map<AssetID, size_t> s_ByID;
	static std::unordered_map<std::string, AssetID> s_ByFile;
	static std::unordered_map<std::string, AssetID> s_Moved;        // files as they were opened: where they were, what they are
	static std::unordered_map<std::string, std::string> s_Absolute; // a path as given -> absolute (it asks the disk)
	static std::unordered_map<std::string, CachedID> s_ReadIDs;     // sidecars and materials read, by their write time
	static std::vector<std::string> s_Orphans;
	static std::vector<std::string> s_Duplicates;
	static std::set<std::string> s_ToldDuplicates;

	static AssetType GetTypeOfFile(const fs::path& file)
	{
		static const std::unordered_map<std::string, AssetType> s_Types = {
			{ ".fbx", AssetType::Model }, { ".obj", AssetType::Model }, { ".gltf", AssetType::Model }, { ".glb", AssetType::Model },
			{ ".dae", AssetType::Model }, { ".3ds", AssetType::Model }, { ".blend", AssetType::Model }, { ".ply", AssetType::Model },
			{ ".stl", AssetType::Model }, { ".x", AssetType::Model }, { ".md5mesh", AssetType::Model },
			{ ".png", AssetType::Texture }, { ".jpg", AssetType::Texture }, { ".jpeg", AssetType::Texture }, { ".tga", AssetType::Texture },
			{ ".bmp", AssetType::Texture }, { ".hdr", AssetType::EnvironmentMap },
			{ ".wav", AssetType::Sound }, { ".ogg", AssetType::Sound }, { ".mp3", AssetType::Sound }, { ".flac", AssetType::Sound },
			{ ".ttf", AssetType::Font }, { ".otf", AssetType::Font },
			{ ".mprefab", AssetType::Prefab }, { ".mmat", AssetType::Material }, { ".manim", AssetType::Animator },
			{ ".mrtex", AssetType::RenderTexture }, { ".mscene", AssetType::Scene },
		};
		std::string extension = file.extension().u8string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		auto found = s_Types.find(extension);
		return found != s_Types.end() ? found->second : AssetType::None;
	}

	static AssetID MakeID()
	{
		static std::mt19937_64 s_Generator(std::random_device{}());
		AssetID id = 0;
		while (id == 0 || s_ByID.find(id) != s_ByID.end())
		{
			id = s_Generator();
		}
		return id;
	}

	static std::string GetAbsolute(const std::string& path)
	{
		auto found = s_Absolute.find(path);
		if (found != s_Absolute.end())
		{
			return found->second;
		}
		std::error_code error;
		const fs::path absolute = fs::weakly_canonical(fs::u8path(path), error);
		const std::string result = (error ? fs::u8path(path) : absolute).generic_u8string();
		s_Absolute[path] = result;
		return result;
	}

	static bool IsInside(const std::string& file, const fs::path& folder)
	{
		const fs::path relative = fs::u8path(file).lexically_relative(folder);
		return !relative.empty() && *relative.begin() != "..";
	}

	// The ID in a sidecar or in a material's file (its first "ID:" line: not worth parsing the material for it)
	static AssetID ReadID(const fs::path& file, bool isMaterial)
	{
		std::error_code error;
		const fs::file_time_type writeTime = fs::last_write_time(file, error);
		if (error)
		{
			return 0;
		}
		const std::string key = file.generic_u8string();
		if (auto found = s_ReadIDs.find(key); found != s_ReadIDs.end() && found->second.WriteTime == writeTime)
		{
			return found->second.ID;
		}
		AssetID id = 0;
		if (isMaterial)
		{
			std::ifstream stream(file);
			for (std::string line; id == 0 && std::getline(stream, line);)
			{
				const size_t start = line.find_first_not_of(' ');
				if (start != std::string::npos && line.compare(start, 3, "ID:") == 0)
				{
					id = std::strtoull(line.c_str() + start + 3, nullptr, 10);
				}
			}
		}
		else
		{
			try
			{
				id = YAML::LoadFile(file.u8string())["Asset"]["ID"].as<AssetID>(0);
			}
			catch (const std::exception& e)
			{
				Log::GetLogger()->warn("Assets: '{0}' can't be read ({1})", key, e.what());
			}
		}
		s_ReadIDs[key] = { writeTime, id };
		return id;
	}

	static bool WriteSidecar(const fs::path& sidecar, AssetID id, AssetType type)
	{
		YAML::Emitter out;
		out << YAML::BeginMap << YAML::Key << "Asset" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Version" << YAML::Value << 1;
		out << YAML::Key << "ID" << YAML::Value << id;
		out << YAML::Key << "Type" << YAML::Value << GetAssetTypeName(type);
		out << YAML::EndMap << YAML::EndMap;
		std::ofstream stream(sidecar);
		if (!stream || !(stream << out.c_str() << "\n"))
		{
			Log::GetLogger()->warn("Assets: '{0}' can't be written", sidecar.generic_u8string());
			return false;
		}
		return true;
	}

	static void AddRecord(const std::string& file, AssetID id, AssetType type)
	{
		Record record;
		record.ID = id;
		record.Type = type;
		record.File = file;
		s_ByID[id] = s_Records.size();
		s_ByFile[file] = id;
		s_Records.push_back(std::move(record));
	}

	// A file with what it is (sidecar read, or made when allowed), into the records. False: it has no ID
	static bool AddFile(const std::string& file, AssetType type, bool makeSidecar)
	{
		std::error_code error;
		const bool isMaterial = type == AssetType::Material;
		const fs::path sidecar = fs::u8path(file + SidecarExtension);
		AssetID id = 0;
		if (isMaterial)
		{
			id = ReadID(fs::u8path(file), true);
		}
		else if (fs::exists(sidecar, error))
		{
			id = ReadID(sidecar, false);
		}
		if (id != 0 && s_ByID.find(id) != s_ByID.end())
		{
			// Copied with its sidecar: the copy is a new asset. A material's ID is in its file: it stays, and is reported
			const std::string other = s_Records[s_ByID[id]].File;
			if (isMaterial || !s_Writable)
			{
				s_Duplicates.push_back(file);
				if (s_ToldDuplicates.insert(file).second)
				{
					Log::GetLogger()->warn("Assets: '{0}' has the ID of '{1}'; it is left out of the registry", file, other);
				}
				return false;
			}
			Log::GetLogger()->warn("Assets: '{0}' had the ID of '{1}' (a copy?): it gets one of its own", file, other);
			id = 0;
			makeSidecar = true;
		}
		if (id == 0)
		{
			if (isMaterial || !makeSidecar || !s_Writable)
			{
				return false;
			}
			id = MakeID();
			if (!WriteSidecar(sidecar, id, type))
			{
				return false;
			}
		}
		AddRecord(file, id, type);
		return true;
	}

	static void Walk(const fs::path& folder, std::vector<std::string>& files)
	{
		std::error_code error;
		if (!fs::is_directory(folder, error))
		{
			return;
		}
		// The folder made absolute once: asking the disk for each file would be most of the walk's time
		for (fs::recursive_directory_iterator it(fs::u8path(GetAbsolute(folder.u8string())), fs::directory_options::skip_permission_denied, error), end; !error && it != end; it.increment(error))
		{
			std::error_code typeError;
			if (it->is_regular_file(typeError))
			{
				files.push_back(it->path().generic_u8string());
			}
		}
		// Of a file and its copy, the shorter name keeps the ID ("Model.obj", not "Model - Copy.obj")
		std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) { return a.size() != b.size() ? a.size() < b.size() : a < b; });
		files.erase(std::unique(files.begin(), files.end()), files.end());
	}

	static void Scan(const std::vector<std::string>& files, bool makeSidecars)
	{
		const std::string extension = SidecarExtension;
		const std::set<std::string> present(files.begin(), files.end());
		for (const std::string& file : files)
		{
			if (file.size() > extension.size() && file.compare(file.size() - extension.size(), extension.size(), extension) == 0)
			{
				if (present.find(file.substr(0, file.size() - extension.size())) == present.end())
				{
					s_Orphans.push_back(file);
				}
				continue;
			}
			const AssetType type = GetTypeOfFile(fs::u8path(file));
			if (type != AssetType::None)
			{
				AddFile(file, type, makeSidecars);
			}
		}
	}

	void Refresh()
	{
		const auto start = std::chrono::steady_clock::now();
		const std::unordered_map<std::string, AssetID> previous = std::move(s_ByFile);
		s_Records.clear();
		s_ByID.clear();
		s_ByFile.clear();
		s_Orphans.clear();
		s_Duplicates.clear();
		s_Absolute.clear();
		s_Scanned = true;

		// The project's first: its files win over a copy elsewhere
		std::vector<std::string> projectFiles, engineFiles;
		if (const Project* project = Project::GetOpen())
		{
			Walk(fs::u8path(project->GetAssetFolder()), projectFiles);
			Walk(fs::u8path(project->GetScenesFolder()), projectFiles);
		}
		std::error_code error;
		const fs::path engine = fs::current_path(error);
		for (const char* folder : { "Models", "Textures", "assets", "Fonts", "Resources" })
		{
			Walk(engine / folder, engineFiles);
		}
		Scan(projectFiles, true);
		Scan(engineFiles, false);
		std::sort(s_Orphans.begin(), s_Orphans.end());

		// What moved: an open scene still has the old paths, and is saved with the asset's ID and its new path
		for (const auto& [file, id] : previous)
		{
			if (auto now = s_ByID.find(id); now != s_ByID.end() && s_Records[now->second].File != file)
			{
				s_Moved[file] = id;
				Log::GetLogger()->info("Assets: '{0}' moved to '{1}'", file, s_Records[now->second].File);
			}
		}
		std::sort(s_Records.begin(), s_Records.end(), [](const Record& a, const Record& b) { return a.File < b.File; });
		for (size_t i = 0; i < s_Records.size(); i++)
		{
			s_ByID[s_Records[i].ID] = i;
		}

		s_LastScan = std::chrono::steady_clock::now();
		s_LastScanMilliseconds = std::chrono::duration<double, std::milli>(s_LastScan - start).count();
		if (previous.empty())
		{
			Log::GetLogger()->info("Assets: {0} with IDs, {1} sidecar(s) without a file, {2:.0f} ms", s_Records.size(), s_Orphans.size(), s_LastScanMilliseconds);
		}
	}

	static void EnsureScanned()
	{
		if (!s_Scanned)
		{
			Refresh();
		}
	}

	void SetWritable(bool writable)
	{
		s_Writable = writable;
	}

	void Invalidate()
	{
		s_Scanned = false;
		s_ByFile.clear();
		s_Moved.clear();
		s_ToldDuplicates.clear();
	}

	AssetID GetID(const std::string& path)
	{
		if (path.empty())
		{
			return 0;
		}
		EnsureScanned();
		const std::string file = GetAbsolute(path);
		if (auto found = s_ByFile.find(file); found != s_ByFile.end())
		{
			return found->second;
		}
		if (auto moved = s_Moved.find(file); moved != s_Moved.end())
		{
			return moved->second;
		}
		std::error_code error;
		const AssetType type = GetTypeOfFile(fs::u8path(file));
		if (type == AssetType::None || !fs::is_regular_file(fs::u8path(file), error))
		{
			return 0;
		}
		// A new file, or one outside the folders walked: a sidecar only for what belongs to the project or the engine
		const Project* project = Project::GetOpen();
		const bool belongs = (project && IsInside(file, fs::u8path(project->GetDirectory()))) || IsInside(file, fs::current_path(error));
		return AddFile(file, type, belongs) ? s_ByFile[file] : 0;
	}

	std::string FindPath(AssetID id)
	{
		if (id == 0)
		{
			return std::string();
		}
		EnsureScanned();
		for (int attempt = 0; attempt < 2; attempt++)
		{
			if (auto found = s_ByID.find(id); found != s_ByID.end())
			{
				Record& record = s_Records[found->second];
				std::error_code error;
				if (fs::is_regular_file(fs::u8path(record.File), error))
				{
					if (record.MemoryPath.empty())
					{
						record.MemoryPath = ProjectPaths::Normalize(record.File);
					}
					return record.MemoryPath;
				}
			}
			// The folders may have changed. Not more than once a second: a scene may ask often
			if (attempt > 0 || std::chrono::steady_clock::now() - s_LastScan < std::chrono::seconds(1))
			{
				break;
			}
			Refresh();
		}
		return std::string();
	}

	const std::vector<Record>& GetRecords()
	{
		EnsureScanned();
		return s_Records;
	}

	const std::vector<std::string>& GetOrphanedSidecars()
	{
		EnsureScanned();
		return s_Orphans;
	}

	const std::vector<std::string>& GetDuplicates()
	{
		EnsureScanned();
		return s_Duplicates;
	}

	double GetLastScanMilliseconds()
	{
		return s_LastScanMilliseconds;
	}
}

namespace AssetReference
{
	static TextMode s_TextMode = TextMode::Full;

	TextMode GetTextMode()
	{
		return s_TextMode;
	}

	ScopedTextMode::ScopedTextMode(TextMode mode)
		: Previous(s_TextMode)
	{
		s_TextMode = mode;
	}

	ScopedTextMode::~ScopedTextMode()
	{
		s_TextMode = Previous;
	}

	void Write(YAML::Emitter& out, const char* key, const std::string& path)
	{
		const AssetID id = s_TextMode == TextMode::Legacy ? 0 : AssetRegistry::GetID(path);
		if (id == 0)
		{
			out << YAML::Key << key << YAML::Value << ProjectPaths::ToStored(path);
			return;
		}
		if (s_TextMode == TextMode::Full)
		{
			const std::string current = AssetRegistry::FindPath(id);
			out << YAML::Key << key << YAML::Value << ProjectPaths::ToStored(current.empty() ? path : current);
		}
		out << YAML::Key << (std::string(key) + "ID") << YAML::Value << id;
	}

	std::string Read(const YAML::Node& node, const char* key, const char* idKey)
	{
		if (!node || !node.IsMap())
		{
			return std::string();
		}
		if (const YAML::Node idNode = node[idKey ? std::string(idKey) : std::string(key) + "ID"])
		{
			const std::string found = AssetRegistry::FindPath(idNode.as<AssetID>(0));
			if (!found.empty())
			{
				return found;
			}
		}
		return ProjectPaths::Resolve(node[key].as<std::string>(""));
	}
}
