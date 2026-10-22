// The scene as a document: New, Open, Save, Save As, unsaved changes, the File menu's shortcuts and the notifications;
// and the project the scenes belong to: New, Open and Close Project


#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanProfiler.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Core/Application.h"
#include "Core/FileDialogs.h"
#include "Core/Input.h"
#include "Core/ResourceManager.h"
#include "Core/Util.h"
#include "H2M/Core/Math/RayH2M.h"
#include "ImGui/ImGuiWrapper.h"
#include "Scene/Scene.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"
#include "Project/Project.h"

#include "imgui.h"
#include "imgui_internal.h" // BeginDragDropTargetCustom (the Models and Meshes panel as one drop area)
#include "ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>


#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

static std::string s_SavedSceneText;          // the scene as last saved, opened or created (unsaved changes: it differs)

// A scene operation requested from the File menu or a shortcut, carried out at the start of the next frame (it replaces
// models and materials the frame may be drawing); each first asks about unsaved changes. The project operations replace
// the scene too.
enum class SceneOperation { None, New, Open, NewProject, OpenProject, CloseProject };
static SceneOperation s_PendingSceneOperation = SceneOperation::None;
static std::string s_PendingScenePath; // Open: the scene file; NewProject: the folder; OpenProject: the project file
static bool s_SceneOperationConfirmed = false; // the unsaved changes question was answered
static bool s_AskAboutUnsavedChanges = false;  // opens the question (see OnImGuiRenderUnsavedChangesPopup)

static std::string SerializeCurrentScene()
{
	return EnvMapVulkanSceneSerializer::Serialize(s_Scene, s_SceneName, s_RenderSettings);
}

// The scene's text for telling whether it has unsaved changes: with the camera at the origin, so that looking around
// isn't a change
static std::string SerializeSceneForComparison()
{
	TransformComponent* cameraTransform = s_Scene.TryGet<TransformComponent>(s_Scene.FindFirst<CameraComponent>());
	const TransformComponent kept = cameraTransform ? *cameraTransform : TransformComponent();
	if (cameraTransform)
	{
		*cameraTransform = TransformComponent();
	}
	std::string text = SerializeCurrentScene();
	if (cameraTransform)
	{
		*cameraTransform = kept;
	}
	return text;
}

// A short message over the bottom of the window that fades away by itself: what a command did when nothing else on the
// screen shows it (saving a scene that had no changes left looks like nothing happened)
static std::string s_NotificationText;
static bool s_NotificationIsError = false;
static double s_NotificationTime = -1000.0;     // ImGui's time when it appeared
static const double NotificationSeconds = 3.0;  // an error stays twice as long
static const double NotificationFadeSeconds = 0.6;

void ShowNotification(const std::string& text, bool isError)
{
	s_NotificationText = text;
	s_NotificationIsError = isError;
	s_NotificationTime = ImGui::GetTime();
}

void OnImGuiRenderNotification()
{
	const double duration = s_NotificationIsError ? NotificationSeconds * 2.0 : NotificationSeconds;
	const double age = ImGui::GetTime() - s_NotificationTime;
	if (s_NotificationText.empty() || age >= duration)
	{
		return;
	}
	const float alpha = (float)glm::clamp((duration - age) / NotificationFadeSeconds, 0.0, 1.0);
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y - 48.0f), ImGuiCond_Always, ImVec2(0.5f, 1.0f));
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.9f * alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize;
	if (ImGui::Begin("##Notification", nullptr, flags))
	{
		ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
		ImGui::TextColored(s_NotificationIsError ? ImVec4(1.0f, 0.45f, 0.4f, 1.0f) : ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "%s", s_NotificationText.c_str());
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
}

// The current scene's text becomes the "saved" state: no unsaved changes
static void MarkSceneSaved()
{
	s_SavedSceneText = SerializeSceneForComparison();
	s_SceneHasUnsavedChanges = false;
}

// Writes the scene file (its file name made by MakeFileName, with the .mscene extension); its materials with unsaved
// changes are saved first, so the file refers to their saved files
static bool SaveSceneTo(const std::string& requestedPath)
{
	std::filesystem::path path(requestedPath);
	path.replace_filename(EnvMapVulkanMaterialLibrary::MakeFileName(path.stem().string(), "Scene") + EnvMapVulkanSceneSerializer::FileExtension);

	// The parts' materials, and the parents of variants (a variant inherits from its parent's saved file when it is
	// loaded), parents first: a variant's file refers to its parent's file
	std::set<EnvMapVulkanMaterial*> visited;
	bool materialsOk = true;
	uint32_t savedCount = 0;
	s_Scene.Each<MeshPartComponent>([&](EnvMapVulkanEntityID, MeshPartComponent& part) {
		std::vector<H2M::RefH2M<EnvMapVulkanMaterial>> chain;
		for (H2M::RefH2M<EnvMapVulkanMaterial> material = part.Material; material && visited.insert(material.Raw()).second;
			material = EnvMapVulkanMaterialLibrary::GetParent(material))
		{
			chain.push_back(material);
		}
		for (auto it = chain.rbegin(); it != chain.rend(); ++it)
		{
			if ((*it)->HasUnsavedChanges())
			{
				const bool ok = EnvMapVulkanMaterialLibrary::Save(*it);
				materialsOk &= ok;
				savedCount += ok ? 1 : 0;
			}
		}
	});
	if (!materialsOk)
	{
		Log::GetLogger()->warn("Scene: a material could not be saved; the scene refers to it by ID only (see above)");
	}

	s_SceneName = path.stem().string();
	const std::string text = SerializeCurrentScene();
	std::error_code error;
	if (!path.parent_path().empty())
	{
		std::filesystem::create_directories(path.parent_path(), error);
	}
	std::ofstream file(path);
	if (!file)
	{
		Log::GetLogger()->error("Scene could not be saved: '{0}' can't be written", path.string());
		ShowNotification("Scene could not be saved: '" + path.string() + "' can't be written", true);
		return false;
	}
	file << text;
	file.close();

	s_SceneFilePath = ProjectPaths::Normalize(path.string());
	MarkSceneSaved();
	const std::string shownPath = ProjectPaths::ToStored(s_SceneFilePath);
	Log::GetLogger()->info("Scene '{0}' saved to '{1}' ({2} entities, {3} material(s) saved with it)", s_SceneName, shownPath,
		s_Scene.GetEntityCount(), savedCount);
	ShowNotification("Scene '" + s_SceneName + "' saved to " + shownPath + (savedCount > 0 ? " (with " + std::to_string(savedCount) +
		(savedCount == 1 ? " material)" : " materials)") : ""), !materialsOk);
	return true;
}

// The open project's scenes folder, else the engine's
static std::string GetScenesFolderForDialog()
{
	const Project* project = Project::GetOpen();
	const std::string folder = project ? project->GetScenesFolder() : std::string(EnvMapVulkanSceneSerializer::ScenesFolder);
	std::error_code error;
	std::filesystem::create_directories(folder, error);
	return std::filesystem::absolute(folder, error).u8string();
}

std::string GetAssetFolderForDialog()
{
	const Project* project = Project::GetOpen();
	return project ? project->GetAssetFolder() : std::string();
}

void SaveSceneAs()
{
	if (RefusedWhilePlaying("Save"))
	{
		return;
	}
	std::string filepath = FileDialogs::SaveFile({ { "Scene", "mscene" } }, GetScenesFolderForDialog());
	if (!filepath.empty())
	{
		SaveSceneTo(filepath);
	}
}

void SaveScene()
{
	if (RefusedWhilePlaying("Save"))
	{
		return;
	}
	if (s_SceneFilePath.empty())
	{
		SaveSceneAs();
	}
	else
	{
		SaveSceneTo(s_SceneFilePath);
	}
}

// What refers to the scene's entities, after its content was cleared or replaced
void ForgetSceneEntities()
{
	s_EditorSelection.SetPrimary(NoEntity);
	s_PendingRemoveEntities.clear();
	s_RenamingEntity = NoEntity;
	EnvMapVulkanRenderer::OnSceneReplaced(); // what the probes captured belongs to the scene that went
	s_SunRotationSynced = false; // the next scene's rotation is where its sun follows the environment from
}

// Everything of the current scene goes (the GPU idle first: its models may be in flight)
static void ClearScene()
{
	EnvMapVulkanRenderer::WaitIdle();
	s_Scene.Clear();
	ForgetSceneEntities();
}

static void NewSceneNow()
{
	ClearScene();
	// The environment's settings back to their defaults (the map stays: loading it again would take a while)
	CreateEnvironmentEntity();
	CreateSunEntity();
	EnsureCameraEntity(); // where the camera is now
	s_PendingSunAlign = true; // the sun on the map's sun, as at startup
	s_SceneFilePath.clear();
	s_SceneName = "Untitled";
	ExtractLights();
	ExtractWater();
	MarkSceneSaved();
	Log::GetLogger()->info("New scene");
}

static bool OpenSceneNow(const std::string& filepath)
{
	// The open scene's models may still be drawn by frames in flight
	EnvMapVulkanRenderer::WaitIdle();
	std::string name, error;
	EnvMapVulkanRenderSettings settings;
	if (!EnvMapVulkanSceneLoader::Open(filepath, s_Scene, name, settings, error))
	{
		Log::GetLogger()->error("Scene '{0}' could not be opened: {1}", filepath, error);
		return false;
	}
	ForgetSceneEntities();

	// A scene always has an environment and a sun
	if (s_Scene.FindFirst<EnvironmentComponent>() == NoEntity)
	{
		CreateEnvironmentEntity();
	}
	if (s_Scene.FindFirst<SunComponent>() == NoEntity)
	{
		CreateSunEntity();
	}
	// And a camera: the scene's own puts the editor's camera where it was saved (see SyncCamera, which finds the entity's
	// transform changed); a scene without one gets one where the camera is now
	const bool hadCamera = s_Scene.FindFirst<CameraComponent>() != NoEntity;
	EnsureCameraEntity();
	if (hadCamera)
	{
		// Also when the saved placement happens to be the one last synced: taken as changed
		s_CameraEntitySynced = true;
		s_SyncedCameraTranslation = glm::vec3(std::numeric_limits<float>::max());
	}
	// The saved sun stays as it is: a map loaded for the scene doesn't turn it to the map's sun
	const EnvironmentComponent& environment = s_Scene.Get<EnvironmentComponent>(s_Scene.FindFirst<EnvironmentComponent>());
	s_PendingSunAlign = false;
	s_KeepSunOnNextEnvironmentLoad = !environment.FilePath.empty() && environment.FilePath != EnvMapVulkanRenderer::GetEnvironmentMapFile();

	s_RenderSettings = settings;
	s_SceneName = name;
	s_SceneFilePath = ProjectPaths::Normalize(filepath);
	ExtractEnvironment();
	ExtractLights();
	ExtractWater();
	MarkSceneSaved();
	Log::GetLogger()->info("Scene '{0}' opened from '{1}': {2} entities", s_SceneName, ProjectPaths::ToStored(s_SceneFilePath),
		s_Scene.GetEntityCount());
	return true;
}

// The editor window's title, with the open project's name after it
static void ShowProjectInWindowTitle()
{
	static std::string s_TitleWithoutProject;
	Window* window = Application::Get()->GetWindow();
	if (s_TitleWithoutProject.empty())
	{
		s_TitleWithoutProject = window->GetTitle();
	}
	const Project* project = Project::GetOpen();
	window->SetTitle(project ? s_TitleWithoutProject + " - " + project->Name : s_TitleWithoutProject);
}

static bool OpenProjectNow(const std::string& filepath)
{
	Project project;
	std::string error;
	if (!Project::Load(filepath, project, error))
	{
		Log::GetLogger()->error("Project '{0}' could not be opened: {1}", filepath, error);
		ShowNotification("Project could not be opened: " + error, true);
		return false;
	}
	Project::SetOpen(project);
	ShowProjectInWindowTitle();
	return true;
}

// The open project's start scene (a new scene when it has none, or it can't be opened)
static void OpenStartSceneNow()
{
	const Project* project = Project::GetOpen();
	const std::string startScene = project ? ProjectPaths::Resolve(project->StartScene) : std::string();
	if (startScene.empty() || !OpenSceneNow(startScene))
	{
		NewSceneNow();
	}
}

// A project in the folder, with a new scene saved as its start scene (Scenes/Main.mscene)
static void NewProjectNow(const std::string& folder)
{
	Project project;
	std::string error;
	if (!Project::Create(folder, project, error))
	{
		Log::GetLogger()->error("Project could not be made in '{0}': {1}", folder, error);
		ShowNotification("Project could not be made: " + error, true);
		return;
	}
	Project::SetOpen(project);
	ShowProjectInWindowTitle();
	NewSceneNow();
	if (SaveSceneTo(project.GetScenesFolder() + "/Main" + EnvMapVulkanSceneSerializer::FileExtension))
	{
		Project::GetOpen()->StartScene = ProjectPaths::ToStored(s_SceneFilePath);
		SaveProject();
	}
	s_ShowProjectPanel = true;
	Log::GetLogger()->info("Project '{0}' made in '{1}'", project.Name, project.GetDirectory());
	ShowNotification("Project '" + project.Name + "' made in " + project.GetDirectory());
}

static void CloseProjectNow()
{
	Project::Close();
	ShowProjectInWindowTitle();
	NewSceneNow();
}

void SaveProject()
{
	std::string error;
	const Project* project = Project::GetOpen();
	if (project && !project->Save(error))
	{
		Log::GetLogger()->error("Project '{0}' could not be saved: {1}", project->Name, error);
		ShowNotification("Project could not be saved: " + error, true);
	}
	ShowProjectInWindowTitle(); // its name may have changed
}

// An operation that replaces the scene: carried out once the unsaved changes question is answered (or there are none)
static void RequestSceneOperation(SceneOperation operation, const std::string& path)
{
	s_PendingSceneOperation = operation;
	s_PendingScenePath = path;
	s_SceneOperationConfirmed = !s_SceneHasUnsavedChanges;
	s_AskAboutUnsavedChanges = s_SceneHasUnsavedChanges;
}

void RequestNewProject()
{
	if (RefusedWhilePlaying("New Project"))
	{
		return;
	}
	const std::string folder = FileDialogs::PickFolder("New Project: a folder for it (the project is named after it)");
	if (!folder.empty())
	{
		RequestSceneOperation(SceneOperation::NewProject, folder);
	}
}

void RequestOpenProject()
{
	if (RefusedWhilePlaying("Open Project"))
	{
		return;
	}
	const std::string filepath = FileDialogs::OpenFile({ { "Project", "mproject" } });
	if (!filepath.empty())
	{
		RequestSceneOperation(SceneOperation::OpenProject, filepath);
	}
}

void RequestCloseProject()
{
	if (!RefusedWhilePlaying("Close Project") && Project::GetOpen())
	{
		RequestSceneOperation(SceneOperation::CloseProject, "");
	}
}

// New and Open, from the File menu or a shortcut (asks about unsaved changes first)
void RequestNewScene()
{
	if (RefusedWhilePlaying("New Scene"))
	{
		return;
	}
	RequestSceneOperation(SceneOperation::New, "");
}

void RequestOpenScene(const std::string& filepath)
{
	if (RefusedWhilePlaying("Open Scene"))
	{
		return;
	}
	std::string path = filepath;
	if (path.empty())
	{
		path = FileDialogs::OpenFile({ { "Scene", "mscene" } }, GetScenesFolderForDialog());
	}
	if (!path.empty())
	{
		RequestSceneOperation(SceneOperation::Open, path);
	}
}

// The question before New or Open drops unsaved changes: Save (then go on), Don't Save, Cancel
void OnImGuiRenderUnsavedChangesPopup()
{
	if (s_AskAboutUnsavedChanges)
	{
		ImGui::OpenPopup("Unsaved Changes##Scene");
		s_AskAboutUnsavedChanges = false;
	}
	if (ImGui::BeginPopupModal("Unsaved Changes##Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Save the changes to the scene '%s'?", s_SceneName.c_str());
		ImGui::TextDisabled("They are lost otherwise.");
		ImGui::Separator();
		if (ImGui::Button("Save", ImVec2(110.0f, 0.0f)))
		{
			SaveScene();
			s_SceneOperationConfirmed = !s_SceneHasUnsavedChanges; // a cancelled Save As keeps the scene
			if (!s_SceneOperationConfirmed)
			{
				s_PendingSceneOperation = SceneOperation::None;
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Don't Save", ImVec2(110.0f, 0.0f)))
		{
			s_SceneOperationConfirmed = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			s_PendingSceneOperation = SceneOperation::None;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// The File menu's shortcuts: Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S. Not while typing in a field, and not while a question
// is open (a popup)
void HandleSceneShortcutKeys()
{
	const ImGuiIO& io = ImGui::GetIO();
	if (!io.KeyCtrl || io.KeyAlt || io.WantTextInput || s_RenamingEntity != NoEntity ||
		ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
	{
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_S, false))
	{
		if (io.KeyShift)
		{
			SaveSceneAs();
		}
		else
		{
			SaveScene();
		}
	}
	else if (!io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_N, false))
	{
		RequestNewScene();
	}
	else if (!io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_O, false))
	{
		RequestOpenScene();
	}
}

// The first entity of this name, in the Scene Hierarchy's order (NoEntity: none)
static EnvMapVulkanEntityID FindEntityByName(const std::vector<EnvMapVulkanEntityID>& entities, const std::string& name)
{
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (s_Scene.Get<NameComponent>(entity).Name == name)
		{
			return entity;
		}
		const EnvMapVulkanEntityID found = FindEntityByName(s_Scene.GetChildren(entity), name);
		if (found != NoEntity)
		{
			return found;
		}
	}
	return NoEntity;
}

// At the start of a frame (EditorPreFrame): New / Open once confirmed
void ProcessSceneOperations()
{
	static bool s_StartupDone = false;
	if (!s_StartupDone)
	{
		s_StartupDone = true;
		MarkSceneSaved(); // the startup scene (environment and sun) has nothing to save yet
		// --debug-view on the command line: the Environment panel's Debug View
		s_ViewOptions.DebugView = glm::clamp(Application::Get()->GetCommandLine().DebugView, 0, (int)DebugViewCount - 1);
		if (DebugViewNeedsRayTracing(s_ViewOptions.DebugView) && !EnvMapVulkanRenderer::IsRayTracingSupported())
		{
			Log::GetLogger()->warn("--debug-view {0} is a ray-traced view: not available without ray tracing", s_ViewOptions.DebugView);
			s_ViewOptions.DebugView = DebugViewOff;
		}
		// --project on the command line: opened like File > Open Project
		const std::string& startupProject = Application::Get()->GetCommandLine().ProjectPath;
		if (!startupProject.empty())
		{
			OpenProjectNow(startupProject);
		}
		// --scene on the command line: opened like File > Open, else the project's start scene (without either the
		// engine starts empty)
		std::string startupScene = Application::Get()->GetCommandLine().ScenePath;
		if (startupScene.empty() && Project::GetOpen())
		{
			startupScene = ProjectPaths::Resolve(Project::GetOpen()->StartScene);
		}
		if (!startupScene.empty())
		{
			RequestOpenScene(startupScene);
		}
		// --play, --simulate: the game starts at the start of the next frame, after the scene opened below
		if (Application::Get()->GetCommandLine().Play)
		{
			RequestPlay();
		}
		else if (Application::Get()->GetCommandLine().Simulate)
		{
			RequestSimulate();
		}
	}
	if (s_PendingSceneOperation == SceneOperation::None || !s_SceneOperationConfirmed)
	{
		return;
	}
	const SceneOperation operation = s_PendingSceneOperation;
	s_PendingSceneOperation = SceneOperation::None;
	s_SceneOperationConfirmed = false;
	if (operation == SceneOperation::New)
	{
		NewSceneNow();
	}
	else if (operation == SceneOperation::NewProject)
	{
		NewProjectNow(s_PendingScenePath);
	}
	else if (operation == SceneOperation::OpenProject)
	{
		if (OpenProjectNow(s_PendingScenePath))
		{
			OpenStartSceneNow();
		}
	}
	else if (operation == SceneOperation::CloseProject)
	{
		CloseProjectNow();
	}
	else
	{
		OpenSceneNow(s_PendingScenePath);
		// --select on the command line: an entity of the startup scene
		static bool s_StartupSelectionDone = false;
		const std::string& selectName = Application::Get()->GetCommandLine().SelectName;
		if (!s_StartupSelectionDone && !selectName.empty())
		{
			s_StartupSelectionDone = true;
			const EnvMapVulkanEntityID entity = FindEntityByName(s_Scene.GetRoots(), selectName);
			if (entity != NoEntity)
			{
				s_EditorSelection.SelectOnly(entity);
			}
			else
			{
				Log::GetLogger()->warn("--select: the scene has no entity named '{0}'", selectName);
			}
		}
	}
}

// Unsaved changes: the scene's text compared with the saved one, a few times a second
void UpdateSceneUnsavedChanges()
{
	if (IsPlaying())
	{
		return; // what the game changes isn't a change of the scene: it stays as it was at Play
	}
	static double s_LastCheck = -1.0;
	const double now = ImGui::GetTime();
	if (now - s_LastCheck < 0.3)
	{
		return;
	}
	s_LastCheck = now;
	s_SceneHasUnsavedChanges = SerializeSceneForComparison() != s_SavedSceneText;
}

void Editor::NewScene()
{
	RequestNewScene();
}

void Editor::OpenScene()
{
	RequestOpenScene();
}

void Editor::SaveScene()
{
	::SaveScene();
}

void Editor::SaveSceneAs()
{
	::SaveSceneAs();
}
