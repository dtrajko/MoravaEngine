#pragma once

#include <map>
#include <string>


struct Project;

/**
 * What a game remembers between its runs (docs/engine/Game_Engine_Runtime_Guide.html, Milestone 2): the player's choices
 * in its Settings menu (volumes, the mouse's sensitivity), as text values by key, in one YAML file per game and user. The
 * C# Settings class reads and writes them; save games are Phase 10's.
 *
 * The game's owner says which file (Open, before the game starts): the player the user's folder (GetUserFile), the
 * editor's Play the project's Library folder (GetEditorFile), so playing in the editor leaves an installed game's
 * settings alone. The values stay while the game loads other scenes; Close saves what changed (the game is over), as
 * Save does at once (C#: Settings.Save).
 */
class GameSettings
{
public:
	static constexpr const char* FileName = "Settings.yaml";

	// <the user's settings folder>/<the project's name>/Settings.yaml: %APPDATA% on Windows, ~/Library/Application Support
	// on macOS, $XDG_CONFIG_HOME (else ~/.config) on Linux. Empty when the folder isn't known
	static std::string GetUserFile(const Project& project);
	// <the project>/Library/Settings.yaml (the Library folder is the editor's, not in Git)
	static std::string GetEditorFile(const Project& project);

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
	const std::string& GetFile() const { return m_File; }
	size_t GetCount() const { return m_Values.size(); }

private:
	std::string m_File;
	std::map<std::string, std::string> m_Values; // sorted: the file's order doesn't change from save to save
	bool m_Changed = false;
};
