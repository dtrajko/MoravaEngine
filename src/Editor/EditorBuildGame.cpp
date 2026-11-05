// File > Build Game (docs/engine/Game_Engine_Runtime_Guide.html, Phase 11): the project's build settings, and the build itself,
// which is this same program started again with --build-game: it cooks and packs without a window, and whatever it does to the
// cook cache or the open project stays out of the running editor. Its log comes here line by line, into the Console.


#include "Assets/GameBuilder.h"
#include "Core/Log.h"
#include "Core/Process.h"
#include "Core/FileDialogs.h"
#include "Project/Project.h"

#include "imgui.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>


#include "EditorState.h"


namespace fs = std::filesystem;

bool s_ShowBuildGamePanel = false;

static std::thread s_Worker;
static std::atomic<bool> s_Building { false };
static std::atomic<bool> s_WorkerDone { false };
static std::mutex s_StatusMutex;
static std::string s_Status;       // the build's last line (under s_StatusMutex)
static std::string s_FirstError;   // (under s_StatusMutex)
static int s_ErrorCount = 0;       // (under s_StatusMutex)
static int s_ExitCode = -1;        // (under s_StatusMutex); -1: no build finished yet
static std::string s_OutputFolder; // empty: the project's default
static std::string s_OutputProject; // the project s_OutputFolder was chosen for

// Each line as the build's log wrote it: "[info] ...", "[warning] ...", "[error] ..."
static void ReadBuildLine(const std::string& line)
{
	std::string text = line;
	spdlog::level::level_enum level = spdlog::level::info;
	const size_t close = line.find("] ");
	if (!line.empty() && line[0] == '[' && close != std::string::npos)
	{
		const std::string name = line.substr(1, close - 1);
		text = line.substr(close + 2);
		level = name == "error" || name == "critical" ? spdlog::level::err : name == "warning" ? spdlog::level::warn : spdlog::level::info;
	}
	if (text.find_first_not_of(" \t") == std::string::npos)
	{
		return;
	}
	Log::GetLogger()->log(level, "{0}", text);

	std::lock_guard<std::mutex> lock(s_StatusMutex);
	s_Status = text;
	if (level == spdlog::level::err)
	{
		s_ErrorCount++;
		if (s_FirstError.empty())
		{
			s_FirstError = text;
		}
	}
}

static void StartBuild(const Project& project, const std::string& outputFolder)
{
	if (s_Worker.joinable())
	{
		s_Worker.join();
	}
	{
		std::lock_guard<std::mutex> lock(s_StatusMutex);
		s_Status = "Starting";
		s_FirstError.clear();
		s_ErrorCount = 0;
		s_ExitCode = -1;
	}
	s_Building = true;
	s_WorkerDone = false;
	s_Worker = std::thread([projectFile = project.FilePath, outputFolder]()
	{
		std::string error;
		const int exitCode = Process::Run(Process::GetExecutablePath().u8string(), { "--build-game", projectFile, "--build-output", outputFolder }, std::string(), &ReadBuildLine, error);
		if (exitCode < 0)
		{
			ReadBuildLine("[error] Build Game: the build could not start: " + error);
		}
		std::lock_guard<std::mutex> lock(s_StatusMutex);
		s_ExitCode = exitCode;
		s_WorkerDone = true;
	});
}

void UpdateBuildGame()
{
	if (s_WorkerDone)
	{
		s_WorkerDone = false;
		s_Worker.join();
		s_Building = false;
		std::lock_guard<std::mutex> lock(s_StatusMutex);
		if (s_ExitCode == 0)
		{
			ShowNotification("The game is built");
		}
		else
		{
			ShowNotification(s_FirstError.empty() ? std::string("The game could not be built: the Console says why") : s_FirstError, true);
		}
	}
}

void ShutdownBuildGame()
{
	if (s_Worker.joinable())
	{
		s_Worker.join(); // the build finishes: left alone, it could be writing the game
	}
}

// A text field for a setting: the setting follows each change, true when an edit ended (save it then)
static bool InputBuildSetting(const char* label, std::string& value, const char* hint)
{
	char buffer[256];
	strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	if (ImGui::InputTextWithHint(label, hint, buffer, sizeof(buffer)))
	{
		value = buffer;
	}
	return ImGui::IsItemDeactivatedAfterEdit();
}

// The scenes the game ships: all of them, or those ticked. The start scene always; a scene that a shipped scene's file names comes
// along by itself, and one named only in a script's code has to be ticked
static bool ShowBuildScenes(Project& project)
{
	bool save = false;
	ImGui::SeparatorText("Scenes");
	bool all = project.Build.Scenes.empty();
	const std::string startScene = project.StartScene.empty() ? std::string() : ProjectPaths::Normalize(ProjectPaths::Resolve(project.StartScene));
	if (ImGui::Checkbox("All the project's scenes", &all))
	{
		project.Build.Scenes.clear();
		if (!all && !startScene.empty())
		{
			project.Build.Scenes.push_back({ project.StartScene, 0 });
		}
		save = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Off: the game has only the scenes ticked below, so test scenes stay out of it.\n"
			"A scene that a shipped scene's file names comes along by itself. One named only in a script's code has to be ticked.");
	}
	if (all)
	{
		return save;
	}

	std::vector<std::string> listed = project.GetBuildScenes();
	const std::vector<std::string>& scenes = GetProjectScenes(project);
	const float height = ImGui::GetTextLineHeightWithSpacing() * (float)std::min<size_t>(scenes.size() + 1, 8) + ImGui::GetStyle().WindowPadding.y * 2.0f;
	if (ImGui::BeginChild("##BuildScenes", ImVec2(0.0f, height), ImGuiChildFlags_Borders))
	{
		const fs::path folder = project.GetScenesFolder();
		for (const std::string& scene : scenes)
		{
			const bool isStart = scene == startScene;
			bool ticked = isStart || std::find(listed.begin(), listed.end(), scene) != listed.end();
			const std::string label = fs::path(scene).lexically_relative(folder).replace_extension().generic_string() + (isStart ? "  (start)" : "") + "##" + scene;
			ImGui::BeginDisabled(isStart);
			if (ImGui::Checkbox(label.c_str(), &ticked))
			{
				auto& stored = project.Build.Scenes;
				const std::string key = ProjectPaths::ToStored(scene);
				stored.erase(std::remove_if(stored.begin(), stored.end(), [&](const ProjectBuildScene& item) { return item.File == key; }), stored.end());
				if (ticked)
				{
					stored.push_back({ key, 0 });
				}
				save = true;
			}
			ImGui::EndDisabled();
		}
	}
	ImGui::EndChild();
	return save;
}

void OnImGuiRenderBuildGamePanel()
{
	if (!s_ShowBuildGamePanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(480.0f, 500.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Build Game", &s_ShowBuildGamePanel))
	{
		ImGui::End();
		return;
	}
	Project* project = Project::GetOpen();
	if (!project)
	{
		ImGui::TextWrapped("A game is built from a project: open one first (File > Open Project).");
		ImGui::End();
		return;
	}

	bool save = false;
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
	save |= InputBuildSetting("Product Name", project->Build.ProductName, project->Name.c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The game's name: its executable (without spaces), its file properties, and its window unless Project Settings gives a title.");
	}
	save |= InputBuildSetting("Company", project->Build.Company, "(none)");
	save |= InputBuildSetting("Version", project->Build.Version, "1.0.0");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Up to four numbers separated by dots");
	}
	save |= InputBuildSetting("Icon", project->Build.Icon, "(none)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A square image, relative to the project folder; 256 x 256 or larger is sharpest.\nThe executable's and the window's icon.");
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("...##Icon"))
	{
		const std::string file = FileDialogs::OpenFile({ { "Images", "png,tga,jpg,jpeg" } }, project->GetAssetFolder());
		if (!file.empty())
		{
			const std::string stored = fs::u8path(file).lexically_proximate(fs::u8path(project->GetDirectory())).generic_u8string();
			project->Build.Icon = stored;
			save = true;
		}
	}
	ImGui::PopItemWidth();

	save |= ShowBuildScenes(*project);

	// Where it goes: chosen for this session, the project's Build folder otherwise
	if (s_OutputProject != project->FilePath)
	{
		s_OutputProject = project->FilePath;
		s_OutputFolder.clear();
	}
	const std::string output = s_OutputFolder.empty() ? GameBuilder::GetDefaultOutputFolder(*project) : s_OutputFolder;
	ImGui::SeparatorText("Output");
	ImGui::TextWrapped("%s", output.c_str());
	if (ImGui::Button("Choose Folder..."))
	{
		const std::string folder = FileDialogs::PickFolder("The folder the game is built into", project->GetDirectory());
		if (!folder.empty())
		{
			s_OutputFolder = folder;
		}
	}
	ImGui::SameLine();
	std::error_code error;
	ImGui::BeginDisabled(!fs::is_directory(fs::u8path(output), error));
	if (ImGui::Button("Open Folder"))
	{
		std::string openError;
		if (!Process::Open(output, openError))
		{
			ShowNotification("The folder can't be opened: " + openError, true);
		}
	}
	ImGui::EndDisabled();

	ImGui::Spacing();
	ImGui::BeginDisabled(s_Building);
	if (ImGui::Button(s_Building ? "Building..." : "Build Game"))
	{
		if (save)
		{
			SaveProject();
			save = false;
		}
		StartBuild(*project, output);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Publishes the scripts, cooks what changed and packs the game, from the files as saved: an unsaved scene goes in as it was last saved.\n"
			"The editor stays usable meanwhile. Errors and warnings are in the Console.");
	}
	{
		std::lock_guard<std::mutex> lock(s_StatusMutex);
		if (s_Building)
		{
			ImGui::TextWrapped("%s", s_Status.c_str());
		}
		else if (s_ExitCode == 0)
		{
			ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Built");
		}
		else if (s_ExitCode > 0 || s_ErrorCount > 0)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.42f, 0.38f, 1.0f));
			ImGui::TextWrapped("%d error(s). %s", s_ErrorCount, s_FirstError.c_str());
			ImGui::PopStyleColor();
		}
	}

	if (save)
	{
		SaveProject();
	}
	ImGui::End();
}
