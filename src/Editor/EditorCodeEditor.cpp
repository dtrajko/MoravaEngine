// The code editor the project's scripts are written in (docs/engine/Game_Engine_Runtime_Guide.html, 4.7): Visual Studio,
// VS Code, Rider, or the program the system has for .csproj and .cs files. The editor opens the scripts' project in it, a
// script at a line (an error in the Console, Edit Script in the Inspector) and a script made with New Script. The
// choice and Build on Save are the editor's settings on this computer, kept in imgui.ini (with the View menu's panels that
// are open).


#include "Project/Project.h"
#include "Scripting/ScriptHost.h"
#include "Scripting/ScriptProjectFiles.h"

#include "Core/Log.h"
#include "Core/Process.h"

#include "imgui.h"
#include "imgui_internal.h" // ImGuiSettingsHandler

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>


#include "EditorState.h"


namespace fs = std::filesystem;

CodeEditorKind s_CodeEditor = CodeEditorKind::Automatic;

const char* GetCodeEditorName(CodeEditorKind kind)
{
	switch (kind)
	{
		case CodeEditorKind::Automatic:    return "Automatic";
		case CodeEditorKind::VisualStudio: return "Visual Studio";
		case CodeEditorKind::VSCode:       return "VS Code";
		case CodeEditorKind::Rider:        return "Rider";
		case CodeEditorKind::System:       return "The System's Program";
		default:                           return "?";
	}
}

// ---- Finding them (once: a code editor installed while the editor runs is found after a restart)

static std::string EnvironmentPath(const char* variable, const char* rest)
{
	const char* value = std::getenv(variable);
	return value ? (fs::u8path(value) / fs::u8path(rest)).u8string() : std::string();
}

static std::string FirstExisting(std::initializer_list<std::string> candidates)
{
	std::error_code error;
	for (const std::string& candidate : candidates)
	{
		if (!candidate.empty() && fs::is_regular_file(fs::u8path(candidate), error))
		{
			return candidate;
		}
	}
	return std::string();
}

static std::string FindVisualStudio()
{
#if defined(_WIN32)
	// vswhere: the installer's tool that says where each Visual Studio is (its devenv.exe)
	const std::string vswhere = FirstExisting({ EnvironmentPath("ProgramFiles(x86)", "Microsoft Visual Studio/Installer/vswhere.exe") });
	if (vswhere.empty())
	{
		return std::string();
	}
	std::string devenv, error;
	Process::Run(vswhere, { "-latest", "-property", "productPath" }, std::string(), [&](const std::string& line)
	{
		if (devenv.empty() && line.size() > 4 && line.compare(line.size() - 4, 4, ".exe") == 0)
		{
			devenv = line;
		}
	}, error);
	return FirstExisting({ devenv });
#else
	return std::string(); // Windows only
#endif
}

static std::string FindVSCode()
{
	std::string code = Process::FindOnPath("code");
	if (!code.empty())
	{
		return code;
	}
#if defined(_WIN32)
	return FirstExisting({ EnvironmentPath("LOCALAPPDATA", "Programs/Microsoft VS Code/bin/code.cmd"),
		EnvironmentPath("ProgramFiles", "Microsoft VS Code/bin/code.cmd") });
#elif defined(__APPLE__)
	return FirstExisting({ "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code" });
#else
	return FirstExisting({ "/usr/bin/code", "/snap/bin/code" });
#endif
}

static std::string FindRider()
{
	std::string rider = Process::FindOnPath("rider");
	if (!rider.empty())
	{
		return rider;
	}
#if defined(_WIN32)
	return FirstExisting({ EnvironmentPath("LOCALAPPDATA", "JetBrains/Toolbox/scripts/rider.cmd") });
#elif defined(__APPLE__)
	return FirstExisting({ "/Applications/Rider.app/Contents/MacOS/rider" });
#else
	return FirstExisting({ EnvironmentPath("HOME", ".local/share/JetBrains/Toolbox/scripts/rider") });
#endif
}

static const std::string& GetCodeEditorProgram(CodeEditorKind kind)
{
	static bool found = false;
	static std::string programs[(int)CodeEditorKind::Count];
	if (!found)
	{
		found = true;
		programs[(int)CodeEditorKind::VisualStudio] = FindVisualStudio();
		programs[(int)CodeEditorKind::VSCode] = FindVSCode();
		programs[(int)CodeEditorKind::Rider] = FindRider();
		for (CodeEditorKind each : { CodeEditorKind::VisualStudio, CodeEditorKind::VSCode, CodeEditorKind::Rider })
		{
			const std::string& program = programs[(int)each];
			Log::GetLogger()->info("Code editors: {0}: {1}", GetCodeEditorName(each), program.empty() ? "not found" : program);
		}
	}
	return programs[(int)kind];
}

bool IsCodeEditorAvailable(CodeEditorKind kind)
{
	return kind == CodeEditorKind::Automatic || kind == CodeEditorKind::System || !GetCodeEditorProgram(kind).empty();
}

// The one used: the chosen one, or (Automatic) the first one found of Visual Studio, VS Code and Rider
static CodeEditorKind ResolveCodeEditor()
{
	if (s_CodeEditor != CodeEditorKind::Automatic)
	{
		return IsCodeEditorAvailable(s_CodeEditor) ? s_CodeEditor : CodeEditorKind::System;
	}
	for (CodeEditorKind kind : { CodeEditorKind::VisualStudio, CodeEditorKind::VSCode, CodeEditorKind::Rider })
	{
		if (IsCodeEditorAvailable(kind))
		{
			return kind;
		}
	}
	return CodeEditorKind::System;
}

const char* GetResolvedCodeEditorName()
{
	return GetCodeEditorName(ResolveCodeEditor());
}

static bool Launch(const std::string& program, const std::vector<std::string>& arguments)
{
	std::string error;
	if (!Process::Start(program, arguments, error))
	{
		Log::GetLogger()->error("Code editor: {0}", error);
		ShowNotification("The code editor could not be started: the log says why", true);
		return false;
	}
	return true;
}

static bool OpenWithSystem(const std::string& path)
{
	std::string error;
	if (!Process::Open(path, error))
	{
		Log::GetLogger()->error("Code editor: {0}", error);
		ShowNotification("The system has no program for '" + fs::u8path(path).filename().u8string() + "': choose a code editor (Project Settings)", true);
		return false;
	}
	return true;
}

bool OpenScriptProjectInCodeEditor()
{
	const std::string csproj = GetScriptProjectPath();
	std::error_code error;
	if (csproj.empty() || !fs::is_regular_file(fs::u8path(csproj), error))
	{
		ShowNotification("The project has no script project (Project Settings: Script Project)", true);
		return false;
	}
	const std::string folder = fs::u8path(csproj).parent_path().u8string();
	const CodeEditorKind kind = ResolveCodeEditor();
	Log::GetLogger()->info("Code editor: '{0}' in {1}", csproj, GetCodeEditorName(kind));
	switch (kind)
	{
		case CodeEditorKind::VisualStudio: return OpenInVisualStudio(GetCodeEditorProgram(kind), csproj, std::string(), 1);
		case CodeEditorKind::VSCode:       return Launch(GetCodeEditorProgram(kind), { folder });
		case CodeEditorKind::Rider:        return Launch(GetCodeEditorProgram(kind), { csproj });
		default:                           return OpenWithSystem(csproj);
	}
}

bool OpenScriptFile(const std::string& file, int line)
{
	// A path relative to the project folder (as the build writes its errors)
	fs::path path = fs::u8path(file);
	if (path.is_relative() && Project::GetOpen())
	{
		path = fs::u8path(Project::GetOpen()->GetDirectory()) / path;
	}
	std::error_code error;
	if (!fs::is_regular_file(path, error))
	{
		ShowNotification("'" + file + "' doesn't exist (any more)", true);
		return false;
	}
	const std::string target = path.lexically_normal().u8string();
	const std::string lineText = std::to_string(std::max(line, 1));
	const CodeEditorKind kind = ResolveCodeEditor();
	switch (kind)
	{
		case CodeEditorKind::VisualStudio:
		{
			// In the Visual Studio of the scripts' project, at the line; without a project, in the one used last (/Edit)
			const std::string csproj = GetScriptProjectPath();
			if (!csproj.empty() && fs::is_regular_file(fs::u8path(csproj), error))
			{
				return OpenInVisualStudio(GetCodeEditorProgram(kind), csproj, target, std::max(line, 1));
			}
			return Launch(GetCodeEditorProgram(kind), { "/Edit", target, "/Command", "Edit.GoTo " + lineText });
		}
		case CodeEditorKind::VSCode:
		{
			// In the window of the scripts' folder (its C# tools need it), at the line
			const std::string csproj = GetScriptProjectPath();
			std::vector<std::string> arguments;
			if (!csproj.empty())
			{
				arguments.push_back(fs::u8path(csproj).parent_path().u8string());
			}
			arguments.push_back("-g");
			arguments.push_back(target + ":" + lineText);
			return Launch(GetCodeEditorProgram(kind), arguments);
		}
		case CodeEditorKind::Rider:
			return Launch(GetCodeEditorProgram(kind), { "--line", lineText, target });
		default:
			return OpenWithSystem(target);
	}
}

// ---- The scripts' files

// The .cs files of the scripts' folder, not the build's (bin/, obj/)
template<typename F>
static void ForEachScriptFile(const fs::path& folder, F&& each)
{
	std::error_code error;
	fs::recursive_directory_iterator it(folder, fs::directory_options::skip_permission_denied, error), end;
	for (; !error && it != end; it.increment(error))
	{
		const std::string name = it->path().filename().string();
		if (it->is_directory(error) && (name == "bin" || name == "obj" || (!name.empty() && name.front() == '.')))
		{
			it.disable_recursion_pending();
		}
		else if (it->path().extension() == ".cs" && !each(it->path()))
		{
			return;
		}
	}
}

static bool IsIdentifierChar(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || (unsigned char)c >= 0x80;
}

bool FindScriptClassFile(const std::string& className, std::string& file, int& line)
{
	const std::string csproj = GetScriptProjectPath();
	if (csproj.empty())
	{
		return false;
	}
	const size_t dot = className.rfind('.');
	const std::string shortName = dot == std::string::npos ? className : className.substr(dot + 1);
	const std::string pattern = "class " + shortName;
	bool found = false;
	ForEachScriptFile(fs::u8path(csproj).parent_path(), [&](const fs::path& path)
	{
		std::ifstream input(path);
		std::string text;
		for (int number = 1; std::getline(input, text); number++)
		{
			for (size_t at = text.find(pattern); at != std::string::npos; at = text.find(pattern, at + 1))
			{
				const size_t after = at + pattern.size();
				if ((at == 0 || !IsIdentifierChar(text[at - 1])) && (after == text.size() || !IsIdentifierChar(text[after])))
				{
					file = path.u8string();
					line = number;
					found = true;
					return false;
				}
			}
		}
		return true;
	});
	return found;
}

// The namespace new scripts go in: the scripts' own (the loaded classes'), else the project's RootNamespace
static std::string GetScriptNamespace(const std::string& csproj)
{
	for (const ScriptClass& scriptClass : ScriptHost::GetClasses())
	{
		const size_t dot = scriptClass.Name.rfind('.');
		if (dot != std::string::npos)
		{
			return scriptClass.Name.substr(0, dot);
		}
	}
	std::ifstream input(fs::u8path(csproj));
	std::stringstream text;
	text << input.rdbuf();
	const std::string content = text.str();
	const size_t start = content.find("<RootNamespace>");
	const size_t end = content.find("</RootNamespace>");
	if (start != std::string::npos && end != std::string::npos && end > start)
	{
		return content.substr(start + 15, end - start - 15);
	}
	return ScriptProjectFiles::MakeIdentifier(fs::u8path(csproj).stem().u8string(), "Game");
}

bool NewScript(const std::string& name, std::string& className, std::string& error)
{
	const std::string csproj = GetScriptProjectPath();
	std::error_code fileError;
	if (csproj.empty() || !fs::is_regular_file(fs::u8path(csproj), fileError))
	{
		error = "the project has no script project (Project Settings: Script Project)";
		return false;
	}
	const std::string namespaceName = GetScriptNamespace(csproj);
	fs::path path;
	if (!ScriptProjectFiles::CreateScript(fs::u8path(csproj).parent_path(), namespaceName, name, path, error))
	{
		return false;
	}
	className = namespaceName + "." + name;
	Log::GetLogger()->info("Scripts: '{0}' made ({1}): it can be chosen once it is built", path.u8string(), className);
	RequestScriptBuild();
	OpenScriptFile(path.u8string(), 1);
	return true;
}

// ---- The settings, in imgui.ini: [MoravaEditor][Settings]

static void* SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
	return std::strcmp(name, "Settings") == 0 ? (void*)1 : nullptr;
}

// The panels of the View menu: open again where they were open
struct PanelSetting
{
	const char* Name;
	bool* Shown;
};
static const PanelSetting s_PanelSettings[] = {
	{ "ShowRendererStats", &s_ShowRendererStats }, { "ShowViewportFps", &s_ShowViewportFps }, { "ShowProject", &s_ShowProjectPanel },
	{ "ShowInput", &s_ShowInputPanel }, { "ShowPhysics", &s_ShowPhysicsPanel }, { "ShowAudio", &s_ShowAudioPanel }, { "ShowAnimator", &s_ShowAnimatorPanel }, { "ShowScripts", &s_ShowScriptsPanel },
	{ "ShowConsole", &s_ShowConsolePanel },
};

static void SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line)
{
	int value = 0;
	for (const PanelSetting& panel : s_PanelSettings)
	{
		const size_t length = std::strlen(panel.Name);
		if (std::strncmp(line, panel.Name, length) == 0 && line[length] == '=')
		{
			*panel.Shown = std::atoi(line + length + 1) != 0;
			return;
		}
	}
	if (std::sscanf(line, "CodeEditor=%d", &value) == 1 && value >= 0 && value < (int)CodeEditorKind::Count)
	{
		s_CodeEditor = (CodeEditorKind)value;
	}
	else if (std::sscanf(line, "BuildScriptsOnSave=%d", &value) == 1)
	{
		s_BuildScriptsOnSave = value != 0;
	}
}

static void SettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buffer)
{
	buffer->appendf("[%s][Settings]\n", handler->TypeName);
	buffer->appendf("CodeEditor=%d\n", (int)s_CodeEditor);
	buffer->appendf("BuildScriptsOnSave=%d\n", s_BuildScriptsOnSave ? 1 : 0);
	for (const PanelSetting& panel : s_PanelSettings)
	{
		buffer->appendf("%s=%d\n", panel.Name, *panel.Shown ? 1 : 0);
	}
	buffer->append("\n");
}

void RegisterEditorSettings()
{
	ImGuiSettingsHandler handler;
	handler.TypeName = "MoravaEditor";
	handler.TypeHash = ImHashStr("MoravaEditor");
	handler.ReadOpenFn = SettingsReadOpen;
	handler.ReadLineFn = SettingsReadLine;
	handler.WriteAllFn = SettingsWriteAll;
	ImGui::AddSettingsHandler(&handler);

	// imgui.ini may have been read already (before the handler was there): its lines again, for this handler
	const char* iniFile = ImGui::GetIO().IniFilename;
	std::ifstream input(iniFile ? iniFile : "");
	std::string line;
	bool inSection = false;
	while (std::getline(input, line))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		if (!line.empty() && line.front() == '[')
		{
			inSection = line == "[MoravaEditor][Settings]";
		}
		else if (inSection)
		{
			SettingsReadLine(nullptr, nullptr, nullptr, line.c_str());
		}
	}
}

void SaveEditorSettings()
{
	ImGui::MarkIniSettingsDirty();
}

// ---- New Script: its name, then the file

static bool s_NewScriptRequested = false;
static EnvMapVulkanEntityID s_NewScriptEntity = NoEntity;
static char s_NewScriptName[64] = {};
static std::string s_NewScriptError;

void OpenNewScriptDialog(EnvMapVulkanEntityID entity)
{
	s_NewScriptRequested = true;
	s_NewScriptEntity = entity;
	s_NewScriptError.clear();
	// A name from the entity's ("Coin 3": Coin), else one to type over
	std::string suggestion = "NewScript";
	if (const NameComponent* name = entity != NoEntity ? s_Scene.TryGet<NameComponent>(entity) : nullptr)
	{
		std::string words = name->Name;
		while (!words.empty() && (words.back() == ' ' || (words.back() >= '0' && words.back() <= '9')))
		{
			words.pop_back();
		}
		suggestion = ScriptProjectFiles::MakeIdentifier(words, suggestion);
	}
	std::snprintf(s_NewScriptName, sizeof(s_NewScriptName), "%s", suggestion.c_str());
}

void OnImGuiRenderNewScriptDialog()
{
	if (s_NewScriptRequested)
	{
		s_NewScriptRequested = false;
		ImGui::OpenPopup("New Script");
	}
	ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("New Script", nullptr, ImGuiWindowFlags_NoSavedSettings))
	{
		return;
	}
	ImGui::TextWrapped("A class deriving from Morava.Entity, in its own file in the scripts' folder. It opens in the code "
		"editor (%s) and is built; once built it can be chosen for any entity.", GetResolvedCodeEditorName());
	if (ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
	}
	const bool entered = ImGui::InputText("Name", s_NewScriptName, sizeof(s_NewScriptName), ImGuiInputTextFlags_EnterReturnsTrue);
	const std::string name = s_NewScriptName;
	const bool valid = ScriptProjectFiles::IsIdentifier(name);
	if (!valid && !name.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "A C# name: letters, digits and _, not starting with a digit");
	}
	else if (s_NewScriptEntity != NoEntity && s_Scene.Exists(s_NewScriptEntity))
	{
		ImGui::TextDisabled("For '%s'", s_Scene.Get<NameComponent>(s_NewScriptEntity).Name.c_str());
	}
	if (!s_NewScriptError.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.42f, 0.38f, 1.0f), "%s", s_NewScriptError.c_str());
	}

	ImGui::BeginDisabled(!valid);
	const bool create = ImGui::Button("Create") || (entered && valid);
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		ImGui::CloseCurrentPopup();
	}
	if (create)
	{
		std::string className;
		if (NewScript(name, className, s_NewScriptError))
		{
			// The entity's Script is the new class already: the Inspector says "not built yet" until it is
			if (s_NewScriptEntity != NoEntity && s_Scene.Exists(s_NewScriptEntity) && !IsPlaying())
			{
				ScriptComponent& script = s_Scene.Has<ScriptComponent>(s_NewScriptEntity) ? s_Scene.Get<ScriptComponent>(s_NewScriptEntity) :
					s_Scene.Add<ScriptComponent>(s_NewScriptEntity);
				script.ClassName = className;
				script.Fields.clear();
			}
			ShowNotification("Script '" + name + "' made: it opens in " + GetResolvedCodeEditorName());
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::EndPopup();
}
