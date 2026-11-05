#include "PackBuilder.h"

#include "AssetFiles.h"
#include "CookFiles.h"
#include "ModelCook.h"
#include "TextureCook.h"

#include "Core/EnginePaths.h"
#include "Core/Log.h"
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/ModelH2M.h"
#include "Project/AssetRegistry.h"
#include "Project/Project.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>


namespace fs = std::filesystem;

namespace PackBuilder
{
	// What the renderer loads by name, whichever scene runs (src/EnvMapVulkan, src/H2M/Renderer/ModelH2M.cpp)
	static const char* const s_EngineFiles[] = {
		ASSETS_DIR "/textures/BRDF_LUT.tga",
		FONTS_DIR "/opensans/OpenSans-Regular.ttf",
		TEXTURES_DIR "/HDR/rooitou_park_4k.hdr",
		TEXTURES_DIR "/dirt.png",
		TEXTURES_DIR "/normal_map_default.png",
		TEXTURES_DIR "/plain.png",
		TEXTURES_DIR "/water/waterNormal.png",
	};

	struct PackFile
	{
		std::string DiskPath;
		bool AsFile = false;
		std::set<AssetPack::Kind> Kinds; // its cooked entries
	};

	static AssetPack::Kind GetEntryKind(TextureCook::MapKind kind)
	{
		switch (kind)
		{
			case TextureCook::MapKind::Color:  return AssetPack::Kind::ColorMap;
			case TextureCook::MapKind::Normal: return AssetPack::Kind::NormalMap;
			default:                           return AssetPack::Kind::DataMap;
		}
	}

	static std::string GetExtension(const std::string& path)
	{
		std::string extension = fs::u8path(path).extension().u8string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return extension;
	}

	static bool IsTextFileWithReferences(const std::string& path)
	{
		static const std::set<std::string> s_Extensions = { ".mscene", ".mmat", ".mprefab", ".manim", ".mrtex", ".minput" };
		return s_Extensions.count(GetExtension(path)) > 0;
	}

	class Builder
	{
	public:
		bool CollectFiles(const Project& project)
		{
			for (const char* file : s_EngineFiles)
			{
				AddFile(file, "the renderer");
			}
			// The project's own folders: scripts may name any of these. An image nothing refers to stays as it is
			std::vector<std::string> unreferencedImages;
			std::vector<std::string> folders = { project.GetAssetFolder(), project.GetScenesFolder() };
			const std::string inputMap = project.GetInputMapPath();
			if (!inputMap.empty())
			{
				folders.push_back(fs::u8path(inputMap).parent_path().generic_u8string());
			}
			for (const std::string& folder : folders)
			{
				std::error_code error;
				for (fs::recursive_directory_iterator it(fs::u8path(folder), fs::directory_options::skip_permission_denied, error), end; !error && it != end; it.increment(error))
				{
					std::error_code typeError;
					if (!it->is_regular_file(typeError) || it->path().extension() == ".meta")
					{
						continue;
					}
					const std::string file = it->path().generic_u8string();
					if (TextureCook::IsCookable(file))
					{
						unreferencedImages.push_back(file);
					}
					else
					{
						AddFile(file, "the project");
					}
				}
			}

			while (!m_Queue.empty())
			{
				const std::string key = m_Queue.front();
				m_Queue.pop_front();
				Process(key);
			}

			for (const std::string& image : unreferencedImages)
			{
				PackFile& file = GetOrAdd(image);
				if (file.Kinds.empty())
				{
					file.AsFile = true;
				}
			}
			m_Queue.clear();
			return m_Errors.empty();
		}

		bool CookMaps()
		{
			for (auto& [key, file] : m_Files)
			{
				for (const auto& [kind, entryKind] : { std::make_pair(TextureCook::MapKind::Color, AssetPack::Kind::ColorMap),
					std::make_pair(TextureCook::MapKind::Normal, AssetPack::Kind::NormalMap), std::make_pair(TextureCook::MapKind::Data, AssetPack::Kind::DataMap) })
				{
					if (file.Kinds.count(entryKind) == 0)
					{
						continue;
					}
					bool cooked = false;
					std::string error;
					if (!TextureCook::CookNow(file.DiskPath, kind, cooked, error))
					{
						Fail("Image '" + key + "' can't be cooked: " + error);
						continue;
					}
					(cooked ? m_MapsCooked : m_MapsCached)++;
				}
			}
			return m_Errors.empty();
		}

		bool Write(const Project& project, const fs::path& folder)
		{
			std::error_code fileError;
			const fs::path file = folder / AssetFiles::DefaultPackName;
			fs::path temporary = file;
			temporary += ".tmp";

			bool ok = false;
			{
				std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
				AssetPack::Writer writer(out);
				ok = out.good() && WriteEntries(project, writer);
				ok = writer.Finish() && ok;
				m_StoredBytes = writer.GetStoredBytes();
				m_OriginalBytes = writer.GetOriginalBytes();
			}
			if (ok)
			{
				fs::rename(temporary, file, fileError);
				ok = !fileError;
			}
			if (!ok)
			{
				fs::remove(temporary, fileError);
				if (m_Errors.empty())
				{
					Fail("The pack '" + file.generic_u8string() + "' could not be written");
				}
			}
			return ok;
		}

		bool CopyScripts(const Project& project, const fs::path& folder)
		{
			const std::string assembly = project.GetScriptAssemblyPath();
			if (assembly.empty())
			{
				return true;
			}
			std::error_code error;
			const fs::path source = fs::u8path(assembly);
			if (!fs::exists(source, error))
			{
				Fail("The script assembly '" + assembly + "' doesn't exist: build the project's scripts first");
				return false;
			}
			// With what it references, beside it. Not the debug symbols, and not what describes running it alone
			for (const fs::directory_entry& entry : fs::directory_iterator(source.parent_path(), error))
			{
				const std::string extension = GetExtension(entry.path().u8string());
				if (extension == ".dll" || entry.path().filename().u8string() == source.stem().u8string() + ".deps.json")
				{
					std::error_code copyError;
					fs::copy_file(entry.path(), folder / entry.path().filename(), fs::copy_options::overwrite_existing, copyError);
					if (copyError)
					{
						Fail("'" + entry.path().generic_u8string() + "' could not be copied: " + copyError.message());
						return false;
					}
				}
			}
			return true;
		}

		void LogSummary(double seconds) const
		{
			Log::GetLogger()->info("Pack: {0} entries, {1:.1f} MB ({2:.1f} MB before compression); models: {3} imported, {4} from the cache; material maps: {5} cooked, {6} from the cache; "
				"{7} shaders ({8} left out); {9:.1f} s", m_Entries, m_StoredBytes / (1024.0 * 1024.0), m_OriginalBytes / (1024.0 * 1024.0), m_ModelsImported, m_ModelsCached, m_MapsCooked, m_MapsCached,
				m_Shaders, m_ShadersLeftOut, seconds);
		}

		void LogErrors() const
		{
			for (const std::string& error : m_Errors)
			{
				Log::GetLogger()->error("Pack: {0}", error);
			}
			Log::GetLogger()->error("Pack: not written, {0} error(s)", m_Errors.size());
		}

		void Fail(const std::string& error) { m_Errors.push_back(error); }

	private:
		PackFile& GetOrAdd(const std::string& path)
		{
			const std::string key = ProjectPaths::ToStored(path);
			auto [it, added] = m_Files.try_emplace(key);
			if (added)
			{
				it->second.DiskPath = path;
				m_Queue.push_back(key);
			}
			return it->second;
		}

		// A file something refers to by name: as it is, or cooked if it is a model
		void AddFile(const std::string& path, const std::string& referredBy)
		{
			if (!AssetFiles::Exists(path))
			{
				Fail(referredBy + " refers to '" + path + "', which doesn't exist");
				return;
			}
			if (fs::u8path(ProjectPaths::ToStored(path)).is_absolute())
			{
				Fail(referredBy + " refers to '" + path + "', outside the project and the engine");
				return;
			}
			PackFile& file = GetOrAdd(path);
			if (!H2M::ModelH2M::IsModelFilePath(path))
			{
				file.AsFile = true;
			}
		}

		void AddMap(const std::string& path, TextureCook::MapKind kind, const std::string& referredBy)
		{
			if (!AssetFiles::Exists(path))
			{
				Fail(referredBy + " refers to the image '" + path + "', which doesn't exist");
				return;
			}
			PackFile& file = GetOrAdd(path);
			if (TextureCook::IsCookable(path) && !TextureCook::IsCompressionOff(path))
			{
				file.Kinds.insert(GetEntryKind(kind));
			}
			else
			{
				file.AsFile = true; // a format that isn't cooked, or its sidecar turns compression off: the map loads from the image
			}
		}

		void Process(const std::string& key)
		{
			const std::string path = m_Files[key].DiskPath;
			if (H2M::ModelH2M::IsModelFilePath(path))
			{
				ProcessModel(key, path);
			}
			else if (IsTextFileWithReferences(path))
			{
				try
				{
					WalkYaml(YAML::LoadFile(path), key, "", "");
				}
				catch (const std::exception& e)
				{
					Fail("'" + key + "' can't be read: " + e.what());
				}
			}
		}

		void ProcessModel(const std::string& key, const std::string& path)
		{
			ModelData data;
			ModelCook::LoadInfo info;
			std::string error;
			if (!ModelCook::Load(path, data, info, error))
			{
				Fail("Model '" + key + "' can't be cooked: " + error);
				return;
			}
			(info.FromCache ? m_ModelsCached : m_ModelsImported)++;
			m_Files[key].Kinds.insert(AssetPack::Kind::Model);

			// Its maps, each in the use ModelH2M loads it for. A missing one isn't an error: the model loads without it, and its
			// file may name what was never on this machine
			auto addMap = [&](const std::string& map, TextureCook::MapKind kind)
			{
				if (map.empty() || map[0] == '*') // '*': embedded in the model file, which ModelH2M doesn't read
				{
					return;
				}
				const std::string resolved = H2M::ModelH2M::ResolveMapPath(path, map);
				if (AssetFiles::Exists(resolved))
				{
					AddMap(resolved, kind, "Model '" + key + "'");
				}
				else
				{
					Log::GetLogger()->warn("Pack: model '{0}' names the map '{1}', which doesn't exist", key, resolved);
				}
			};
			for (const ModelMaterialData& material : data.Materials)
			{
				addMap(material.AlbedoMap, TextureCook::MapKind::Color);
				addMap(material.NormalMap, TextureCook::MapKind::Normal);
				addMap(material.ShininessMap, TextureCook::MapKind::Data);
				addMap(material.DiffuseRoughnessMap, TextureCook::MapKind::Data);
				addMap(material.ReflectionFactorMap, TextureCook::MapKind::Data);
				addMap(material.MetalnessMap, TextureCook::MapKind::Data);
				addMap(material.EmissiveMap, TextureCook::MapKind::Color);
				addMap(material.AmbientOcclusionMap, TextureCook::MapKind::Data);
			}
		}

		// The references in a saved file. Every file kind writes them the same way (AssetReference), so one walk reads them all: a
		// key with a <key>ID beside it, or a value that names a file of an asset's type (a script field's clip). Names and texts
		// may look like file names and aren't references
		void WalkYaml(const YAML::Node& node, const std::string& file, const std::string& parentKey, const std::string& grandparentKey)
		{
			if (node.IsSequence())
			{
				for (const YAML::Node& item : node)
				{
					WalkYaml(item, file, parentKey, grandparentKey);
				}
				return;
			}
			if (!node.IsMap())
			{
				return;
			}
			for (const auto& pair : node)
			{
				const std::string key = pair.first.as<std::string>("");
				if (!pair.second.IsScalar())
				{
					WalkYaml(pair.second, file, key, parentKey);
					continue;
				}
				const std::string value = pair.second.as<std::string>("");
				const bool hasID = bool(node[key + "ID"]);
				const bool isText = key.find("Name") != std::string::npos || key.find("Text") != std::string::npos || key.find("Title") != std::string::npos;
				const bool isTemplate = value.find('{') != std::string::npos; // a script's "Footstep_{0}.wav": the project's files are all in the pack
				if (value.empty() || isText || isTemplate || (!hasID && GetAssetTypeOfFile(value) == AssetType::None))
				{
					continue;
				}
				// A material's own ID goes by ID (Parent: {ID, File}, and a part's material)
				const std::string path = AssetReference::Read(node, key.c_str(), !hasID && key == "File" && node["ID"] ? "ID" : nullptr);
				const std::string referredBy = "'" + file + "' (" + key + ")";
				TextureCook::MapKind kind = TextureCook::MapKind::Data;
				if (grandparentKey == "Maps" && EnvMapVulkanMaterial::FindMapKind(parentKey, kind))
				{
					AddMap(path, kind, referredBy);
				}
				else
				{
					AddFile(path, referredBy);
				}
			}
		}

		void AddEntry(AssetPack::Writer& writer, AssetID id, AssetType type, AssetPack::Kind kind, const std::string& path, const std::vector<uint8_t>& bytes)
		{
			writer.Add(id, type, kind, path, bytes.data(), bytes.size());
			m_Entries++;
		}

		bool WriteEntries(const Project& project, AssetPack::Writer& writer)
		{
			// The project as the pack's game has it: its assembly beside the pack, and nothing to build
			YAML::Node root = YAML::LoadFile(project.FilePath);
			if (!project.ScriptAssembly.empty())
			{
				root["Project"]["ScriptAssembly"] = fs::u8path(project.ScriptAssembly).filename().u8string();
			}
			root["Project"].remove("ScriptProject");
			YAML::Emitter emitter;
			emitter << root;
			const std::string projectText = emitter.c_str();
			AddEntry(writer, 0, AssetType::None, AssetPack::Kind::File, AssetFiles::PackProjectFile, std::vector<uint8_t>(projectText.begin(), projectText.end()));

			// In a fixed order: the same assets make the same pack
			for (const auto& [key, file] : m_Files)
			{
				const AssetID id = AssetRegistry::FindID(file.DiskPath);
				const AssetType type = GetAssetTypeOfFile(file.DiskPath);
				std::vector<uint8_t> bytes;
				if (file.AsFile)
				{
					if (!CookFiles::ReadWholeFile(fs::u8path(file.DiskPath), bytes))
					{
						Fail("'" + key + "' can't be read");
						return false;
					}
					AddEntry(writer, id, type, AssetPack::Kind::File, key, bytes);
				}
				for (const AssetPack::Kind kind : file.Kinds)
				{
					const std::string cooked = kind == AssetPack::Kind::Model ? ModelCook::GetCookedFile(file.DiskPath) :
						TextureCook::GetCookedFile(file.DiskPath, kind == AssetPack::Kind::ColorMap ? TextureCook::MapKind::Color :
							kind == AssetPack::Kind::NormalMap ? TextureCook::MapKind::Normal : TextureCook::MapKind::Data);
					if (!CookFiles::ReadWholeFile(fs::u8path(cooked), bytes))
					{
						Fail("The cooked file of '" + key + "' can't be read (" + cooked + ")");
						return false;
					}
					AddEntry(writer, id, type, kind, key, bytes);
				}
			}

			// All the renderer's shaders: which of them a game needs depends on its scenes' settings, and they are small
			std::vector<std::string> shaders;
			std::error_code error;
			for (const fs::directory_entry& entry : fs::directory_iterator(fs::u8path(RESOURCE_SHADERS_DIR), error))
			{
				if (GetExtension(entry.path().u8string()) == ".glsl")
				{
					shaders.push_back(entry.path().generic_u8string());
				}
			}
			std::sort(shaders.begin(), shaders.end());
			for (const std::string& shader : shaders)
			{
				std::vector<uint8_t> bytes;
				std::string shaderError;
				if (!H2M::VulkanShaderH2M::Cook(shader, bytes, shaderError))
				{
					// The folder also holds the older scenes' shaders, which aren't Vulkan's. One the renderer loads compiles in the
					// editor every day, and a game missing one says so when it starts
					Log::GetLogger()->warn("Pack: shader '{0}' left out, it doesn't compile for Vulkan: {1}", shader, shaderError);
					m_ShadersLeftOut++;
					continue;
				}
				AddEntry(writer, 0, AssetType::None, AssetPack::Kind::Shader, ProjectPaths::ToStored(shader), bytes);
				m_Shaders++;
			}
			return true;
		}

		std::map<std::string, PackFile> m_Files; // by the path a saved file writes
		std::deque<std::string> m_Queue;
		std::vector<std::string> m_Errors;
		uint32_t m_ModelsImported = 0, m_ModelsCached = 0, m_MapsCooked = 0, m_MapsCached = 0, m_Shaders = 0, m_ShadersLeftOut = 0, m_Entries = 0;
		uint64_t m_StoredBytes = 0, m_OriginalBytes = 0;
	};

	int Run(const std::string& projectFile, const std::string& outputFolder)
	{
		const auto start = std::chrono::steady_clock::now();
		Project project;
		std::string error;
		if (!Project::Load(projectFile, project, error))
		{
			Log::GetLogger()->error("Pack: the project '{0}' could not be opened: {1}", projectFile, error);
			return 1;
		}
		Project::SetOpen(project);
		const fs::path folder = fs::u8path(outputFolder.empty() ? project.GetDirectory() + "/" + Project::BuildFolder : outputFolder);
		Log::GetLogger()->info("Pack: building project '{0}' into '{1}'", project.Name, folder.generic_u8string());

		std::error_code folderError;
		fs::create_directories(folder, folderError);

		Builder builder;
		const bool ok = builder.CollectFiles(project) && builder.CookMaps() && builder.CopyScripts(project, folder) && builder.Write(project, folder);
		if (!ok)
		{
			builder.LogErrors();
			return 1;
		}
		builder.LogSummary(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
		return 0;
	}
}
