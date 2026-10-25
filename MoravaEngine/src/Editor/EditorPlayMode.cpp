// Play mode (docs/engine/Game_Engine_Runtime_Guide.html, 1.5): Play, Simulate, Pause, Step and Stop, and their toolbar
//
// The game runs on s_Scene, the scene every panel and the renderer show, so they all show the running game; the edited
// scene waits in a copy (s_EditedScene) and comes back at Stop. It is the same as running the game on a copy: nothing the
// game does reaches the edited scene. While the game runs, the view is the scene's camera entity, the game loop runs once
// per frame, and the viewport belongs to the game (no camera flying, picking or gizmo). Paused, the game loop stops and
// the viewport is the editor's again: fly around, select, inspect, Step one frame at a time.
//
// Input (2.3): the game gets the keyboard, the mouse and the gamepads only while the Viewport panel has the focus (a click
// on it gives it). The mouse is captured only when the game asks for it; Escape (Shift+F1 when the game binds Escape) always
// gives the focus, and so the mouse, back to the editor.
//
// Simulate (3.7): the same copy of the scene and the same game loop, with physics but without the game's logic (the input
// test and the scripts) and without input. The view stays the editor's camera and the viewport keeps its tools
// (select, gizmo, the Inspector shows the bodies moving), as while paused. At Stop the view stays where it is.
//
// Scripts (4.5): a script that throws pauses Play, with a notification (the exception is in the log with its file and
// line), so the scene is there to look at as it was. Play on, or Step, carries on.
//
// Scene.Load (6.2): another scene's file opened into s_Scene at the end of the frame, and the game goes on in it; the
// edited scene still waits in its copy. Scene.Reload of the scene Play started with takes it from that copy, as it was at
// Play (unsaved changes too), without loading its models again.


#include "Audio/AudioEngine.h"

#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"

#include "Project/Project.h"

#include "H2M/Renderer/ModelH2M.h"

#include "imgui.h"
#include "imgui_internal.h" // the menu bar's rectangle

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <chrono>
#include <utility>


#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

// What the toolbar asked for, done at the start of the next frame (the renderer may be drawing the scene now)
enum class PlayRequest { None, Play, Simulate, Step, Stop };
static PlayRequest s_PlayRequest = PlayRequest::None;

// Edit: no game; Play: the game; Simulate: physics only, in the editor's view. Play and Simulate can be paused.
enum class PlayState { Edit, Play, Simulate };
static PlayState s_PlayState = PlayState::Edit;
static bool s_Paused = false;
static GameLoop s_GameLoop;
static GameSystems s_GameSystems;
static InputSystem s_GameInput;
static bool s_GameViewportFocused = false; // the Viewport panel had the focus in the last frame: the game gets input
static GLFWwindow* s_GameWindow = nullptr;  // the OS window the Viewport panel is in (the main one, or its own when dragged out)
static bool s_FocusViewport = false;       // Play: the Viewport panel gets the focus, so the game has input at once
static uint64_t s_ScriptExceptionsSeen = 0; // Play pauses when a script throws: the ones it paused for already

// The edited scene while the game runs: the scene, its render settings, and the matrices of its models' meshes. The
// models are shared with the game (CopyFrom doesn't load them again), and a part's transform is written into its mesh
// (the renderer's ApplyPartTransforms, the gizmo), so the meshes' matrices are kept too.
static EnvMapVulkanScene s_EditedScene;
static EnvMapVulkanRenderSettings s_EditedRenderSettings;
static std::vector<std::pair<H2M::RefH2M<H2M::ModelH2M>, std::vector<glm::mat4>>> s_EditedMeshTransforms;
static std::string s_EditedSceneText; // the edited scene as text: at Stop, the scene that came back must be the same
static std::string s_PlayedScenePath; // a scene the game loaded (Scene.Load), empty: the edited one

// The game's view: the editor's view options without the grid and the selection's overlays
static EnvMapVulkanViewOptions s_GameViewOptions;

bool IsPlaying()
{
	return s_PlayState != PlayState::Edit;
}

bool IsSimulating()
{
	return s_PlayState == PlayState::Simulate;
}

bool IsGameRunning()
{
	return s_PlayState == PlayState::Play && !s_Paused;
}

void RequestPlay()
{
	s_PlayRequest = PlayRequest::Play;
}

void RequestSimulate()
{
	s_PlayRequest = PlayRequest::Simulate;
}

bool RefusedWhilePlaying(const char* what)
{
	if (!IsPlaying())
	{
		return false;
	}
	ShowNotification(std::string(what) + " isn't possible while the game runs: Stop first", true);
	return true;
}

// The editor's view when Play started: back at Stop (6.3)
static EnvMapVulkanSceneSerializer::EditorView s_ViewBeforePlay;

// The view of the primary game camera, with its lens: the game's view (none: the editor's view stays)
static void PlaceGameCamera(H2M::CameraH2M* camera)
{
	const EnvMapVulkanEntityID cameraEntity = FindPrimaryCamera(s_Scene);
	if (cameraEntity != NoEntity)
	{
		EnvMapVulkanSceneData::PlaceGameCamera(*camera, s_Scene, cameraEntity);
	}
}

static std::string SerializeScene()
{
	return EnvMapVulkanSceneSerializer::Serialize(s_Scene, s_SceneName, s_RenderSettings);
}

static void StartPlay(H2M::CameraH2M* camera, PlayState state)
{
	ClearConsoleIfAskedOnPlay();

	// The editor's view, for Stop: Play shows the primary game camera's
	s_ViewBeforePlay = GetEditorView();
	if (state == PlayState::Play && FindPrimaryCamera(s_Scene) == NoEntity)
	{
		ShowNotification("The scene has no camera: the game is seen from the editor's view (Scene Hierarchy: Add > Camera)", true);
	}

	s_EditedScene.CopyFrom(s_Scene);
	s_EditedRenderSettings = s_RenderSettings;
	s_EditedSceneText = SerializeScene();
	s_EditedMeshTransforms.clear();
	s_PlayedScenePath.clear();
	for (EnvMapVulkanEntityID model : GetModels(s_Scene))
	{
		H2M::RefH2M<H2M::ModelH2M> modelRef = GetModel(s_Scene, model);
		std::vector<glm::mat4> transforms;
		for (const H2M::RefH2M<H2M::MeshH2M>& mesh : modelRef->GetMeshes())
		{
			transforms.push_back(mesh->Transform);
		}
		s_EditedMeshTransforms.emplace_back(modelRef, std::move(transforms));
	}

	s_PlayState = state;
	s_Paused = false;
	s_GameLoop.Reset();
	s_ScriptExceptionsSeen = 0;
	s_GameSystems.SetSceneInfo(s_SceneName, s_SceneFilePath);
	if (state == PlayState::Play)
	{
		// The project's input file is read at each Play: an edit of it counts at the next Play
		s_GameInput.SetMap(GameSystems::LoadInputMap());
		s_GameSystems.Begin(&s_Scene, &s_GameInput);
		s_FocusViewport = true;
		PlaceGameCamera(camera);
	}
	else
	{
		s_GameSystems.Begin(&s_Scene, nullptr); // no input: physics only
	}
	Log::GetLogger()->info("{0}: scene '{1}', {2} entities", state == PlayState::Play ? "Play" : "Simulate", s_SceneName, s_Scene.GetEntityCount());
}

// The edited scene into s_Scene, as it was at Play: its entities, its render settings and its models' meshes' matrices
// (the models are shared: the game moved their parts). The GPU is idle (the game's models go)
static void CopyEditedScene()
{
	s_Scene.CopyFrom(s_EditedScene);
	s_RenderSettings = s_EditedRenderSettings;
	for (auto& [model, transforms] : s_EditedMeshTransforms)
	{
		std::vector<H2M::RefH2M<H2M::MeshH2M>>& meshes = model->GetMeshes();
		for (size_t i = 0; i < meshes.size() && i < transforms.size(); i++)
		{
			meshes[i]->Transform = transforms[i];
		}
	}
}

// Scene.Load (6.2), at the end of the frame: path (absolute), or empty for the scene being played again. The edited scene
// again comes from its copy; a file that can't be opened leaves the game as it is
static void LoadSceneInPlay(const std::string& requested)
{
	const auto start = std::chrono::steady_clock::now();
	const std::string path = requested.empty() ? s_PlayedScenePath : ProjectPaths::Normalize(requested);
	EnvMapVulkanRenderer::WaitIdle(); // the game's models may still be drawn by frames in flight
	std::string name;
	if (path.empty())
	{
		CopyEditedScene();
		name = s_SceneName;
	}
	else
	{
		std::string error;
		EnvMapVulkanRenderSettings settings;
		if (!EnvMapVulkanSceneLoader::Open(path, s_Scene, name, settings, error))
		{
			Log::GetLogger()->error("Scene.Load: '{0}' could not be opened: {1} (the game goes on in this scene)", ProjectPaths::ToStored(path), error);
			ShowNotification("Scene.Load: " + ProjectPaths::ToStored(path) + " could not be opened (the log says why)", true);
			return;
		}
		s_RenderSettings = settings;
	}
	s_PlayedScenePath = path;

	// As when a scene opens: nothing selected, the probes start again, and the sun stays as the scene has it when the
	// scene brings another map (a map loaded for it doesn't turn the sun to the map's)
	ForgetSceneEntities();
	const EnvironmentComponent* environment = s_Scene.TryGet<EnvironmentComponent>(s_Scene.FindFirst<EnvironmentComponent>());
	s_PendingSunAlign = false;
	s_KeepSunOnNextEnvironmentLoad = environment && !environment->FilePath.empty() &&
		environment->FilePath != EnvMapVulkanRenderer::GetEnvironmentMapFile();
	ExtractEnvironment();
	ExtractLights();
	ExtractWater();

	s_GameSystems.SetSceneInfo(name, path.empty() ? s_SceneFilePath : path);
	s_GameSystems.Restart();
	Log::GetLogger()->info("Scene.Load: scene '{0}'{1}: {2} entities, {3:.0f} ms", name,
		path.empty() ? " (the edited one, as at Play)" : " from '" + ProjectPaths::ToStored(path) + "'", s_Scene.GetEntityCount(),
		std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
}

// After a frame of the game: the scene a script asked for
static void ProcessSceneLoad()
{
	std::string path;
	if (s_GameSystems.TakeSceneLoad(path))
	{
		LoadSceneInPlay(path);
	}
}

// camera: placed where the edited scene's camera entity is (nullptr: at exit, no view to restore)
static void StopPlay()
{
	// Models the game loaded may still be drawn by frames in flight
	EnvMapVulkanRenderer::WaitIdle();

	// The mouse back to the editor
	s_GameInput.Reset(s_GameWindow ? s_GameWindow : Application::Get()->GetWindow()->GetHandle());
	s_GameSystems.Begin(nullptr, nullptr);
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

	// A map the game (or a panel) changed: the edited scene's comes back with its sun as it is, not turned to the map's sun
	const EnvironmentComponent* playedEnvironment = s_Scene.TryGet<EnvironmentComponent>(s_Scene.FindFirst<EnvironmentComponent>());
	const std::string playedMap = playedEnvironment ? playedEnvironment->FilePath : std::string();
	const bool playedSunExtracted = playedEnvironment && playedEnvironment->ExtractSun;

	CopyEditedScene();
	s_EditedScene.Clear();
	s_EditedMeshTransforms.clear();
	s_PlayedScenePath.clear();

	const EnvironmentComponent* environment = s_Scene.TryGet<EnvironmentComponent>(s_Scene.FindFirst<EnvironmentComponent>());
	if (environment && (environment->FilePath != playedMap || environment->ExtractSun != playedSunExtracted))
	{
		s_KeepSunOnNextEnvironmentLoad = true;
	}

	// What the editor and the renderer keep about the scene: the environment's rotation the sun follows from (the game may
	// have turned it; else the sun would be turned back with it), and the light the probes gathered from the game's scene
	s_SunRotationSynced = false;
	EnvMapVulkanRenderer::OnSceneReplaced();

	// What referred to entities only the game had
	s_EditorSelection.Sync();
	s_PendingRemoveEntities.clear();
	if (!s_Scene.Exists(s_RenamingEntity))
	{
		s_RenamingEntity = NoEntity;
	}

	// Play: the editor's view as before Play. Simulate: the view stays where it is (it was the editor's all along)
	if (s_PlayState == PlayState::Play)
	{
		SetEditorView(s_ViewBeforePlay);
	}

	s_PlayState = PlayState::Edit;
	s_Paused = false;
	const bool unchanged = SerializeScene() == s_EditedSceneText;
	s_EditedSceneText.clear();
	if (unchanged)
	{
		Log::GetLogger()->info("Stop: {0} frames, {1} fixed steps, {2:.2f} s of game time; the edited scene is back, unchanged",
			s_GameLoop.GetFrameCount(), s_GameLoop.GetFixedStepCount(), s_GameLoop.GetGameTime());
	}
	else
	{
		Log::GetLogger()->error("Stop: the scene that came back differs from the scene at Play (something the game changed wasn't restored)");
	}

	const uint32_t released = ResourceManager::PurgeUnusedTextures2D(); // the textures of models the game loaded
	if (released > 0)
	{
		Log::GetLogger()->info("{0} unused texture(s) released", released);
	}
}

// A script threw (C# logged it with the file and line): the game pauses, so the error and the scene are there to look at.
// Play on, or Step, carries on with the scripts as they are
static void PauseOnScriptException()
{
	const ScriptRuntime& scripts = s_GameSystems.GetScripts();
	if (scripts.GetExceptionCount() == s_ScriptExceptionsSeen)
	{
		return;
	}
	s_ScriptExceptionsSeen = scripts.GetExceptionCount();
	if (!s_Paused)
	{
		s_Paused = true;
		Log::GetLogger()->warn("Play paused: {0} threw an exception", scripts.GetLastException());
	}
	ShowNotification(scripts.GetLastException() + " threw an exception: Play is paused (the log has the file and line)", true);
}

void UpdatePlayMode(H2M::CameraH2M* camera, float frameTime)
{
	const PlayRequest request = s_PlayRequest;
	s_PlayRequest = PlayRequest::None;
	if ((request == PlayRequest::Play || request == PlayRequest::Simulate) && !IsPlaying())
	{
		StartPlay(camera, request == PlayRequest::Play ? PlayState::Play : PlayState::Simulate);
	}
	else if (request == PlayRequest::Stop && IsPlaying())
	{
		StopPlay();
		return;
	}
	if (!IsPlaying())
	{
		return;
	}
	// Paused, the sounds stop where they are (a Step doesn't play them: its frame is too short to hear)
	AudioEngine::SetPaused(s_Paused);

	// Simulate: the game loop alone (no input, the editor's view)
	if (s_PlayState == PlayState::Simulate)
	{
		if (!s_Paused)
		{
			s_GameLoop.Frame(frameTime, &s_GameSystems);
		}
		else if (request == PlayRequest::Step)
		{
			s_GameLoop.Frame(GameLoop::FixedStep, &s_GameSystems);
		}
		return;
	}

	// Input while the game runs and the viewport has the focus (paused, the viewport is the editor's: a Step gets none),
	// from the OS window the viewport is in
	GLFWwindow* window = s_GameWindow ? s_GameWindow : Application::Get()->GetWindow()->GetHandle();
	const bool gameInput = !s_Paused && s_GameViewportFocused;
	if (!s_Paused)
	{
		s_GameInput.Update(window, gameInput, frameTime);
		s_GameLoop.Frame(frameTime, &s_GameSystems);
		ProcessSceneLoad();
		PlaceGameCamera(camera); // where the game put the camera entity
		PauseOnScriptException();
	}
	else
	{
		const bool step = request == PlayRequest::Step;
		// One frame of exactly one fixed step, whatever this frame's time: Step after Step gives the same steps
		s_GameInput.Update(window, false, step ? GameLoop::FixedStep : frameTime);
		if (step)
		{
			s_GameLoop.Frame(GameLoop::FixedStep, &s_GameSystems);
			ProcessSceneLoad();
			PauseOnScriptException();
		}
	}

	// A captured mouse moves without a cursor: ImGui leaves it alone (a click would land where the hidden cursor drifted)
	ImGuiIO& io = ImGui::GetIO();
	if (s_GameInput.IsMouseCaptured())
	{
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
	}
	else
	{
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}

}

GLFWwindow* GetCurrentPlatformWindow()
{
	const ImGuiViewport* viewport = ImGui::GetWindowViewport();
	GLFWwindow* window = viewport ? static_cast<GLFWwindow*>(viewport->PlatformHandle) : nullptr;
	return window ? window : Application::Get()->GetWindow()->GetHandle();
}

const InputSystem* GetGameInput()
{
	return s_PlayState == PlayState::Play ? &s_GameInput : nullptr;
}

ScenePhysics* GetGamePhysics()
{
	return IsPlaying() ? &s_GameSystems.GetPhysics() : nullptr;
}

SceneAudio* GetGameAudio()
{
	return s_PlayState == PlayState::Play ? &s_GameSystems.GetAudio() : nullptr;
}

ScriptRuntime* GetGameScripts()
{
	return s_PlayState == PlayState::Play ? &s_GameSystems.GetScripts() : nullptr;
}

// The game binds Escape: Shift+F1 gives the mouse back instead
static bool GameBindsEscape()
{
	return s_GameInput.GetMap().BindsKey(GLFW_KEY_ESCAPE);
}

static const char* GetReleaseKeyName()
{
	return GameBindsEscape() ? "Shift+F1" : "Escape";
}

void UpdateGameViewportFocus(bool viewportFocused)
{
	// Its OS window: changes when the panel is dragged out of the main window or back (the mouse goes back to the old one)
	GLFWwindow* window = GetCurrentPlatformWindow();
	if (window != s_GameWindow && s_GameWindow)
	{
		s_GameInput.Reset(s_GameWindow);
	}
	s_GameWindow = window;

	if (s_FocusViewport)
	{
		ImGui::SetWindowFocus(); // the current window: the viewport
		s_FocusViewport = false;
		viewportFocused = true;
	}
	s_GameViewportFocused = viewportFocused && IsGameRunning();
	if (!s_GameViewportFocused)
	{
		return;
	}
	const ImGuiIO& io = ImGui::GetIO();
	const bool release = GameBindsEscape() ? io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F1, false) :
		ImGui::IsKeyPressed(ImGuiKey_Escape, false) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F1, false));
	if (release)
	{
		ImGui::SetWindowFocus(nullptr); // no panel has the focus: the game has no input, and the mouse is the editor's
		s_GameViewportFocused = false;
	}
}

void HandlePlayModeShortcutKeys()
{
	const ImGuiIO& io = ImGui::GetIO();
	if (!io.KeyCtrl || io.KeyAlt || io.WantTextInput || s_RenamingEntity != NoEntity ||
		ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
	{
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_P, false))
	{
		s_PlayRequest = IsPlaying() ? PlayRequest::Stop : (io.KeyShift ? PlayRequest::Simulate : PlayRequest::Play);
	}
}

void ShutdownPlayMode()
{
	if (IsPlaying())
	{
		StopPlay();
	}
}

const EnvMapVulkanViewOptions* GetDrawnViewOptions()
{
	if (!IsGameRunning())
	{
		return &s_ViewOptions;
	}
	// The editor's choices that suit a game too stay (the debug view, the shadow views)
	s_GameViewOptions = s_ViewOptions;
	s_GameViewOptions.Grid = false;
	s_GameViewOptions.Overlays = EnvMapVulkanViewOptions().Overlays;
	s_GameViewOptions.Selection.clear();
	return &s_GameViewOptions;
}

// The toolbar's icons, drawn in a button's square
enum class ToolbarIcon { Play, Simulate, Stop, Pause, Step };

static void DrawToolbarIcon(ToolbarIcon icon, const ImVec2& min, const ImVec2& max)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text); // faded by BeginDisabled
	const float size = max.x - min.x;
	const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
	const float half = size * 0.22f;
	switch (icon)
	{
		case ToolbarIcon::Play:
			drawList->AddTriangleFilled(ImVec2(center.x - half * 0.8f, center.y - half), ImVec2(center.x - half * 0.8f, center.y + half),
				ImVec2(center.x + half, center.y), color);
			break;
		case ToolbarIcon::Simulate:
			// A ball falling onto the ground: physics
			drawList->AddCircleFilled(ImVec2(center.x, center.y - half * 0.3f), half * 0.55f, color);
			drawList->AddRectFilled(ImVec2(center.x - half, center.y + half * 0.6f), ImVec2(center.x + half, center.y + half * 0.9f), color);
			break;
		case ToolbarIcon::Stop:
			drawList->AddRectFilled(ImVec2(center.x - half * 0.85f, center.y - half * 0.85f), ImVec2(center.x + half * 0.85f, center.y + half * 0.85f), color);
			break;
		case ToolbarIcon::Pause:
			drawList->AddRectFilled(ImVec2(center.x - half * 0.8f, center.y - half), ImVec2(center.x - half * 0.25f, center.y + half), color);
			drawList->AddRectFilled(ImVec2(center.x + half * 0.25f, center.y - half), ImVec2(center.x + half * 0.8f, center.y + half), color);
			break;
		case ToolbarIcon::Step:
			drawList->AddTriangleFilled(ImVec2(center.x - half, center.y - half), ImVec2(center.x - half, center.y + half),
				ImVec2(center.x + half * 0.35f, center.y), color);
			drawList->AddRectFilled(ImVec2(center.x + half * 0.45f, center.y - half), ImVec2(center.x + half * 0.9f, center.y + half), color);
			break;
	}
}

// The toolbar's buttons, and their margin from the menu bar's top and bottom edges: the menu bar is made taller for them
// (GetMenuBarFramePadding)
static float GetToolbarButtonSize()
{
	return IM_ROUND(ImGui::GetFontSize() * 1.8f);
}

static float GetToolbarMargin()
{
	return IM_ROUND(ImGui::GetFontSize() * 0.35f);
}

ImVec2 GetMenuBarFramePadding()
{
	const float barHeight = GetToolbarButtonSize() + 2.0f * GetToolbarMargin();
	return ImVec2(ImGui::GetStyle().FramePadding.x, (barHeight - ImGui::GetFontSize()) * 0.5f);
}

// A square button with an icon; active: highlighted (the game runs, or is paused)
static bool ToolbarButton(const char* id, ToolbarIcon icon, bool active, bool enabled, const char* tooltip)
{
	const float size = GetToolbarButtonSize();
	ImGui::BeginDisabled(!enabled);
	if (active)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
	}
	const bool clicked = ImGui::Button(id, ImVec2(size, size));
	if (active)
	{
		ImGui::PopStyleColor();
	}
	DrawToolbarIcon(icon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s", tooltip);
	}
	return clicked;
}

void OnImGuiRenderPlayToolbar()
{
	// Centered in the menu bar, the margin below its top edge
	const ImRect bar = ImGui::GetCurrentWindow()->MenuBarRect();
	const float size = GetToolbarButtonSize();
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float width = 4.0f * size + 3.0f * spacing;
	// Each button at its own position: SameLine would put the next one on the menu bar's text line, not this button's
	float x = std::max(ImGui::GetCursorScreenPos().x, IM_ROUND(bar.GetCenter().x - width * 0.5f));
	const float y = bar.Min.y + GetToolbarMargin();
	auto nextButton = [&]()
	{
		ImGui::SetCursorScreenPos(ImVec2(x, y));
		x += size + spacing;
	};
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

	// Play and Simulate: each becomes Stop while it runs, and can't start while the other runs
	const bool playing = s_PlayState == PlayState::Play;
	const bool simulating = s_PlayState == PlayState::Simulate;
	nextButton();
	if (ToolbarButton("##PlayStop", playing ? ToolbarIcon::Stop : ToolbarIcon::Play, playing, !simulating,
		playing ? "Stop (Ctrl+P): back to the edited scene, as it was at Play" : "Play (Ctrl+P): the game runs on a copy of the scene, seen through its camera"))
	{
		s_PlayRequest = playing ? PlayRequest::Stop : PlayRequest::Play;
	}
	nextButton();
	if (ToolbarButton("##SimulateStop", simulating ? ToolbarIcon::Stop : ToolbarIcon::Simulate, simulating, !playing,
		simulating ? "Stop (Ctrl+Shift+P): back to the edited scene, as it was at Simulate" :
			"Simulate (Ctrl+Shift+P): physics only, without the game's logic and input, on a copy of the scene; the view and the tools stay the editor's"))
	{
		s_PlayRequest = simulating ? PlayRequest::Stop : PlayRequest::Simulate;
	}
	nextButton();
	if (ToolbarButton("##Pause", ToolbarIcon::Pause, s_Paused, IsPlaying(),
		s_Paused ? "Go on (the game is paused: look around, select, inspect)" : "Pause: the game stops where it is"))
	{
		s_Paused = !s_Paused;
	}
	nextButton();
	if (ToolbarButton("##Step", ToolbarIcon::Step, false, IsPlaying() && s_Paused, "Step: one frame of the paused game (one fixed step of 1/60 s)"))
	{
		s_PlayRequest = PlayRequest::Step;
	}
	ImGui::PopStyleVar();

	// The game loop's counters (1.4): a Step adds exactly one frame
	if (IsPlaying())
	{
		ImGui::SetCursorScreenPos(ImVec2(x, bar.Min.y)); // the menu bar centers its text from its top
		ImGui::TextDisabled("%sFrame %llu  %.2f s%s", simulating ? "Simulate  " : "", (unsigned long long)s_GameLoop.GetFrameCount(),
			s_GameLoop.GetGameTime(), s_Paused ? "  (paused)" : "");
	}
}

void DrawPlayModeOutline(const ImVec2& imageMin, const ImVec2& imageSize)
{
	if (!IsPlaying() || imageSize.x <= 0.0f || imageSize.y <= 0.0f)
	{
		return;
	}
	// Play: blue, as the active Play button; Simulate: green; paused: amber
	const ImU32 color = s_Paused ? IM_COL32(230, 160, 40, 255) :
		(IsSimulating() ? IM_COL32(70, 190, 110, 255) : ImGui::GetColorU32(ImGuiCol_ButtonActive, 1.0f));
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRect(imageMin, ImVec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y), color, 0.0f, 0, 3.0f);

	// Who has the input, at the bottom of the view (Simulate: nobody, the view is the editor's)
	if (s_Paused || IsSimulating())
	{
		return;
	}
	char text[128];
	if (!s_GameViewportFocused)
	{
		snprintf(text, sizeof(text), "Click the view: the game gets the input");
	}
	else if (s_GameInput.IsMouseCaptured())
	{
		snprintf(text, sizeof(text), "%s: the mouse back to the editor", GetReleaseKeyName());
	}
	else
	{
		snprintf(text, sizeof(text), "The game has the input (%s: back to the editor)", GetReleaseKeyName());
	}
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const ImVec2 position(IM_ROUND(imageMin.x + (imageSize.x - textSize.x) * 0.5f), IM_ROUND(imageMin.y + imageSize.y - textSize.y - 12.0f));
	drawList->AddRectFilled(ImVec2(position.x - 6.0f, position.y - 3.0f), ImVec2(position.x + textSize.x + 6.0f, position.y + textSize.y + 3.0f),
		IM_COL32(0, 0, 0, 140), 4.0f);
	drawList->AddText(position, s_GameViewportFocused ? IM_COL32(235, 235, 235, 255) : IM_COL32(255, 200, 90, 255), text);
}
