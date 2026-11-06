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

// A name as a folder name: characters a file system refuses become _
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

// <folder>/<Company>/<Product> (no company: <folder>/<Product>); empty when the folder isn't known
static std::string GetGameFolder(const std::string& folder, const Project& project)
{
	if (folder.empty())
	{
		return folder;
	}
	fs::path path = fs::u8path(folder);
	if (!project.Build.Company.empty())
	{
		path /= fs::u8path(ToFolderName(project.Build.Company));
	}
	return (path / fs::u8path(ToFolderName(project.GetProductName()))).generic_u8string();
}

GameSettings::GameSettings(const char* section)
	: m_Section(section)
{
}

std::string GameSettings::GetUserFolder(const Project& project)
{
	return GetGameFolder(GetUserSettingsFolder(), project);
}

std::string GameSettings::GetUserDataFolder(const Project& project)
{
	std::string folder;
#if defined(_WIN32)
	const wchar_t* localAppData = _wgetenv(L"LOCALAPPDATA");
	folder = localAppData && *localAppData ? fs::path(localAppData).u8string() : std::string();
#elif defined(__APPLE__)
	folder = GetUserSettingsFolder();
#else
	const char* data = std::getenv("XDG_DATA_HOME");
	const char* home = std::getenv("HOME");
	folder = data && *data ? std::string(data) : (home && *home ? (fs::u8path(home) / ".local/share").u8string() : std::string());
#endif
	return GetGameFolder(folder, project);
}

std::string GameSettings::GetUserFile(const Project& project, const char* name)
{
	const std::string folder = GetUserFolder(project);
	return folder.empty() ? std::string() : (fs::u8path(folder) / name).generic_u8string();
}

std::string GameSettings::GetEditorFile(const Project& project, const char* name)
{
	return (fs::u8path(project.GetDirectory()) / "Library" / name).generic_u8string();
}

PlayerSettings GameSettings::ReadPlayerSettings(const std::string& file)
{
	std::error_code exists;
	if (file.empty() || !fs::exists(fs::u8path(file), exists))
	{
		return PlayerSettings();
	}
	try
	{
		std::ifstream stream(fs::u8path(file));
		return PlayerSettings::Read(YAML::Load(stream)[PlayerSettings::Section]);
	}
	catch (const std::exception&)
	{
		return PlayerSettings(); // Open says why, when the game opens it
	}
}

void GameSettings::Open(const std::string& file)
{
	m_File = file;
	m_Values.clear();
	m_Player = PlayerSettings();
	m_Changed = false;
	std::error_code exists;
	if (file.empty() || !fs::exists(fs::u8path(file), exists))
	{
		Log::GetLogger()->info("{0}: {1}", m_Section, file.empty() ? "for this run only (no folder for them)" : "none yet ('" + file + "')");
		return;
	}
	try
	{
		std::ifstream stream(fs::u8path(file));
		const YAML::Node root = YAML::Load(stream);
		const YAML::Node values = root[m_Section];
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
		m_Player = PlayerSettings::Read(root[PlayerSettings::Section]);
		Log::GetLogger()->info("{0}: {1} from '{2}'{3}", m_Section, m_Values.size(), file, m_Player.IsEmpty() ? "" : ", and the player's settings");
	}
	catch (const std::exception& e)
	{
		Log::GetLogger()->error("{0}: '{1}' could not be read: {2} (the game starts without them)", m_Section, file, e.what());
	}
}

void GameSettings::Close()
{
	Save();
	m_File.clear();
	m_Values.clear();
	m_Player = PlayerSettings();
	m_Changed = false;
}

bool GameSettings::Save()
{
	if (!m_Changed || m_File.empty())
	{
		return true;
	}
	YAML::Emitter out;
	out << YAML::BeginMap;
	if (!m_Player.IsEmpty())
	{
		out << YAML::Key << PlayerSettings::Section << YAML::Value;
		m_Player.Write(out);
	}
	out << YAML::Key << m_Section << YAML::Value << YAML::BeginMap;
	for (const auto& [key, value] : m_Values)
	{
		out << YAML::Key << key << YAML::Value << value;
	}
	out << YAML::EndMap << YAML::EndMap;

	// A new file in the old one's place (a rename is all or nothing: the old one stays until the new one is complete)
	const fs::path path = fs::u8path(m_File);
	const fs::path written = fs::u8path(m_File + ".new");
	std::error_code error;
	fs::create_directories(path.parent_path(), error);
	{
		std::ofstream stream(written, std::ios::trunc);
		stream << out.c_str() << "\n";
		stream.flush();
		if (!stream)
		{
			Log::GetLogger()->error("{0}: '{1}' could not be written", m_Section, m_File);
			return false;
		}
	}
	fs::rename(written, path, error);
	if (error)
	{
		Log::GetLogger()->error("{0}: '{1}' could not be replaced: {2}", m_Section, m_File, error.message());
		fs::remove(written, error);
		return false;
	}
	m_Changed = false;
	Log::GetLogger()->trace("{0}: {1}{2} saved to '{3}'", m_Section, m_Values.size(), m_Player.IsEmpty() ? "" : " and the player's settings", m_File);
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

void GameSettings::RemoveAll()
{
	if (!m_Values.empty())
	{
		m_Values.clear();
		m_Changed = true;
	}
}
