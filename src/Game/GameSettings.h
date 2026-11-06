#pragma once

#include "PlayerSettings.h"

#include <map>
#include <string>


struct Project;

/**
 * What a game remembers between its runs (docs/engine/Game_Engine_Runtime_Guide.html, Milestone 2 and 11.4), in a YAML file
 * per game and user. Two of them:
 * - the settings (Settings.yaml): the player's choices that the engine puts in place (PlayerSettings: the window, the
 *   quality, the volumes, the bindings), and the game's own (the C# Settings class: the mouse's sensitivity), text values by
 *   key in the Settings section
 * - the save data (SaveData.yaml, the C# SaveData class): the game's progress, text values by key in the SaveData section
 *
 * The game's owner says which file (Open, before the game starts): the player the user's folder (GetUserFile), the
 * editor's Play the project's Library folder (GetEditorFile), so playing in the editor leaves an installed game's files
 * alone. The values stay while the game loads other scenes; Close saves what changed (the game is over), as Save does at
 * once (C#: Settings.Save, SaveData.Save). A save writes a new file and then puts it in the old one's place, so a game
 * stopped in the middle of it leaves the old file, not half of the new one.
 */
class GameSettings
{
public:
	static constexpr const char* SettingsFile = "Settings.yaml";
	static constexpr const char* SaveDataFile = "SaveData.yaml";
	static constexpr const char* SettingsSection = "Settings";
	static constexpr const char* SaveDataSection = "SaveData";

	// The section the values are kept in (SettingsSection, SaveDataSection)
	explicit GameSettings(const char* section);

	// What a game keeps of the user's and would follow them to another machine (its settings, its save data):
	// <Company>/<Product> in %APPDATA% on Windows, ~/Library/Application Support on macOS, $XDG_CONFIG_HOME (else ~/.config)
	// on Linux. Empty when the folder isn't known
	static std::string GetUserFolder(const Project& project);
	// A shipped game's log, crash dumps and caches, not worth carrying to another machine: <Company>/<Product> in %LOCALAPPDATA% on
	// Windows, ~/Library/Application Support on macOS, $XDG_DATA_HOME (else ~/.local/share) on Linux. Empty when not known
	static std::string GetUserDataFolder(const Project& project);
	// <the user folder>/<name> (SettingsFile, SaveDataFile); empty when the folder isn't known
	static std::string GetUserFile(const Project& project, const char* name);
	// <the project>/Library/<name> (the Library folder is the editor's, not in Git)
	static std::string GetEditorFile(const Project& project, const char* name);

	// The player's choices in a settings file, read without opening it for the game (the player's window, before it opens)
	static PlayerSettings ReadPlayerSettings(const std::string& file);

	// The values from the file (none when it doesn't exist yet; a file that can't be read is logged and left as it is
	// until a value changes). An empty file: values for this run only
	void Open(const std::string& file);
	// Saves what changed and forgets the values
	void Close();
	// Writes the values when they changed since the last save: false (logged) when the file can't be written
	bool Save();

	bool Get(const std::string& key, std::string& value) const;
	void Set(const std::string& key, const std::string& value);
	bool Remove(const std::string& key); // false: there was no such key
	void RemoveAll();                    // the game's values (not the player's settings)
	const std::string& GetFile() const { return m_File; }
	size_t GetCount() const { return m_Values.size(); }

	// The player's choices (the settings file's; the save data has none). Edit: they changed, the next save writes them
	const PlayerSettings& GetPlayer() const { return m_Player; }
	PlayerSettings& EditPlayer() { m_Changed = true; return m_Player; }

private:
	const char* m_Section;
	std::string m_File;
	std::map<std::string, std::string> m_Values; // sorted: the file's order doesn't change from save to save
	PlayerSettings m_Player;
	bool m_Changed = false;
};
