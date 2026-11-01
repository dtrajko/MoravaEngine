#include "GameSettings.h"

#include "Project/Project.h"

#include "Core/Log.h"

#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>


namespace fs = std::filesystem;

// The folder each platform keeps applications' settings in, UTF-8 (empty: not known)
static std::string GetUserSettingsFolder()
{
#if defined(_WIN32)
	const wchar_t* appData = _wgetenv(L"APPDATA");
	return appData && *appData ? fs::path(appData).u8string() : std::string();
#else
	const char* home = std::getenv("HOME");
#if defined(__APPLE__)
	return home && *home ? (fs::u8path(home) / "Library/Application Support").u8string() : std::string();
#else
	const char* config = std::getenv("XDG_CONFIG_HOME");
	if (config && *config)
	{
		return config;
	}
	return home && *home ? (fs::u8path(home) / ".config").u8string() : std::string();
#endif
#endif
}

// The project's name as a folder name: characters a file system refuses become _
static std::string ToFolderName(const std::string& name)
{
	std::string folder;
	for (char c : name)
	{
		const bool refused = c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' ||
			c == '|' || (unsigned char)c < 32;
		folder += refused ? '_' : c;
	}
	while (!folder.empty() && (folder.back() == '.' || folder.back() == ' '))
	{
		folder.pop_back(); // Windows drops them
	}
	return folder.empty() ? "Untitled" : folder;
}

std::string GameSettings::GetUserFile(const Project& project)
{
	const std::string folder = GetUserSettingsFolder();
	return folder.empty() ? std::string() : (fs::u8path(folder) / fs::u8path(ToFolderName(project.Name)) / FileName).generic_u8string();
}

std::string GameSettings::GetEditorFile(const Project& project)
{
	return (fs::u8path(project.GetDirectory()) / "Library" / FileName).generic_u8string();
}

void GameSettings::Open(const std::string& file)
{
	m_File = file;
	m_Values.clear();
	m_Changed = false;
	std::error_code exists;
	if (file.empty() || !fs::exists(fs::u8path(file), exists))
	{
		Log::GetLogger()->info("Settings: {0}", file.empty() ? "for this run only (no folder for them)" : "none yet ('" + file + "')");
		return;
	}
	try
	{
		std::ifstream stream(fs::u8path(file));
		const YAML::Node root = YAML::Load(stream);
		const YAML::Node values = root["Settings"];
		if (values && values.IsMap())
		{
			for (const auto& entry : values)
			{
				if (entry.second.IsScalar())
				{
					m_Values[entry.first.as<std::string>()] = entry.second.as<std::string>();
				}
			}
		}
		Log::GetLogger()->info("Settings: {0} from '{1}'", m_Values.size(), file);
	}
	catch (const std::exception& e)
	{
		Log::GetLogger()->error("Settings: '{0}' could not be read: {1} (the game starts without them)", file, e.what());
	}
}

void GameSettings::Close()
{
	Save();
	m_File.clear();
	m_Values.clear();
	m_Changed = false;
}

bool GameSettings::Save()
{
	if (!m_Changed || m_File.empty())
	{
		return true;
	}
	YAML::Emitter out;
	out << YAML::BeginMap << YAML::Key << "Settings" << YAML::Value << YAML::BeginMap;
	for (const auto& [key, value] : m_Values)
	{
		out << YAML::Key << key << YAML::Value << value;
	}
	out << YAML::EndMap << YAML::EndMap;

	const fs::path path = fs::u8path(m_File);
	std::error_code error;
	fs::create_directories(path.parent_path(), error);
	std::ofstream stream(path);
	stream << out.c_str() << "\n";
	if (!stream)
	{
		Log::GetLogger()->error("Settings: '{0}' could not be written", m_File);
		return false;
	}
	m_Changed = false;
	Log::GetLogger()->trace("Settings: {0} saved to '{1}'", m_Values.size(), m_File);
	return true;
}

bool GameSettings::Get(const std::string& key, std::string& value) const
{
	const auto found = m_Values.find(key);
	if (found == m_Values.end())
	{
		return false;
	}
	value = found->second;
	return true;
}

void GameSettings::Set(const std::string& key, const std::string& value)
{
	const auto [entry, added] = m_Values.try_emplace(key, value);
	if (added || entry->second != value)
	{
		entry->second = value;
		m_Changed = true;
	}
}

bool GameSettings::Remove(const std::string& key)
{
	if (m_Values.erase(key) == 0)
	{
		return false;
	}
	m_Changed = true;
	return true;
}
