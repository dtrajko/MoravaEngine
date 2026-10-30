#pragma once

// The editor's internals shared by its files (Editor.cpp, EditorViewport.cpp, EditorDocument.cpp...): its state and the
// functions one file calls in another. Not for code outside src/Editor: the editor's interface is Editor.h.
// The variables keep their s_ names: there is one of each, the editor's.

#include "Editor.h"
#include "EditorSelection.h"

#include "Game/GameLoop.h"
#include "Game/GameSystems.h"

#include "Input/InputSystem.h"

#include "EnvMapVulkan/EnvMapVulkanLights.h"
#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Core/Window.h"

#include "imgui.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>


using EnvMapVulkanSceneData::LightEntities;
using EnvMapVulkanSceneData::LightKind;

template<typename T>
bool Contains(const std::vector<T>& values, const T& value)
{
	return std::find(values.begin(), values.end(), value) != values.end();
}

// What an entity is, by its components (see GetEntityKind)
enum class EntityKind { Other, Environment, Sun, Camera, PointLight, SpotLight, Water, ProbeVolume, Model, Part, Sound, UICanvas, UIElement };

// A map assigned or removed in the Material Editor (button or drag & drop), applied at the start of the next frame (see EditorPreFrame)
struct PendingMaterialTexture
{
	H2M::RefH2M<EnvMapVulkanMaterial> Material;
	uint32_t Slot;        // EnvMapVulkanMaterial::Map
	std::string FilePath; // empty: remove the map
};

// ---- State (defined in EditorState.cpp)

// The scene being edited, how it is drawn (saved with it) and the view's options (the debug views, the grid, the overlays,
// the selection): the renderer draws them (EnvMapVulkanRenderer::SetScene)
extern EnvMapVulkanScene s_Scene;
extern EnvMapVulkanRenderSettings s_RenderSettings;
extern EnvMapVulkanViewOptions s_ViewOptions;
// The debug overlay (7.4) over the viewport: F3, the Environment panel, --debug-overlay
extern bool s_DebugOverlay;
// The editor's camera (the viewport's): given to OnUpdate every frame
extern H2M::CameraH2M* s_Camera;
extern EditorSelection s_EditorSelection;

// The panels' and gizmos' copies of the scene's environment, lights, water and probe volume, read every frame (ExtractLights,
// ExtractWater...): the editing code edits a copy and writes it back (CommitSun, CommitWater...). The selected light: the
// selected entity as a kind and an index into s_Lights' lists; the water is selected when its entity is.
extern EnvMapVulkanEntityID s_EnvironmentEntity;
extern EnvMapVulkanLightEnvironment s_Lights;
extern LightEntities s_LightEntities;
extern LightKind s_SelectedLightKind;
extern int s_SelectedLightIndex;
extern EnvMapVulkanWaterSettings s_WaterSettings;
extern EnvMapVulkanEntityID s_WaterEntity;
extern bool s_WaterSelected;
extern EnvMapVulkanProbeVolumeSettings s_ProbeVolumeSettings;
extern EnvMapVulkanEntityID s_ProbeVolumeEntity;

// The editor's view's field of view (View > Editor Camera; saved with the scene's EditorCamera)
extern float s_EditorFieldOfView;
// The sun follows the environment's rotation (SyncSunWithEnvironmentRotation): false makes it start over from the rotation now
extern bool s_SunRotationSynced;
// The sun is turned to the environment map's sun at the start of the next frame (startup, a sun extracted); not for the
// map of a scene just opened (its saved sun stays)
extern bool s_PendingSunAlign;
extern bool s_KeepSunOnNextEnvironmentLoad;

// What the panels ask for, done at the start of the next frame (EditorPreFrame), when the renderer records nothing
extern std::string s_PendingModelFilename;                   // a model to load
extern std::optional<glm::vec3> s_PendingModelGroundPosition; // dropped on the viewport: where the model stands (see LoadModel)
extern std::vector<EnvMapVulkanEntityID> s_PendingRemoveEntities; // models and parts to remove
extern bool s_PendingPaste;
extern H2M::RefH2M<EnvMapVulkanMaterial> s_PendingDeleteMaterial;
extern std::vector<PendingMaterialTexture> s_PendingMaterialTextures;
extern std::vector<std::string> s_PendingMaterialFiles;      // material files to load (a file may replace a material in use)

// The scene's file (empty: never saved, "Untitled"), its name, and whether it differs from the file
extern std::string s_SceneFilePath;
extern std::string s_SceneName;
extern bool s_SceneHasUnsavedChanges;

// The panels' shared choices
extern H2M::RefH2M<EnvMapVulkanMaterial> s_SelectedMaterial; // edited in the Material Editor
extern EnvMapVulkanEntityID s_RenamingEntity;                // the entity whose name is being edited in the hierarchy
extern char s_RenameBuffer[128];
extern bool s_ShowLightGizmos;                               // light icons and shapes in the viewport
extern bool s_ShowAudioGizmos;                               // sound icons and the selected sound's distances (Audio panel)
extern bool s_ShowRendererStats;
extern bool s_ShowProjectPanel;
extern bool s_ShowInputPanel;
extern bool s_ShowPhysicsPanel;
extern bool s_ShowAudioPanel;                                // View > Audio (EditorAudioPanel.cpp)
extern bool s_ShowPhysicsColliders;                          // physics debug views in the viewport (the Physics panel)
extern bool s_ShowPhysicsContacts;
extern bool s_ShowViewportFps;
extern bool s_ViewportFullscreen; // F11: the viewport alone, the window covering its monitor (the panels hidden, not closed)

// Drag & drop payload of a Material Library material: an index into EnvMapVulkanMaterialLibrary::GetMaterials()
constexpr const char* s_MaterialPayload = "VULKAN_LIBRARY_MATERIAL";

// The clipboard (EditorClipboard.cpp): something to paste, and emptied (at shutdown)
bool CanPaste();
void ClearClipboard();

// ---- The frame (Editor.cpp)

// The environment map's rotation (degrees around Y), the scene's environment entity's
float GetEnvironmentRotation();

bool IsEnvironmentMapFile(const std::string& filepath);

// Accepts a file dragged from the Content Browser over the current drop target, if accepts(path) is true. While dragging, an
// accepted file outlines the drop area (highlightMin..highlightMax) and any other file shows what can be dropped there
// (expected, e.g. "a model file"). Returns true once an accepted file is dropped, with its path in filepath.
bool AcceptFileDrop(const ImVec2& highlightMin, const ImVec2& highlightMax, bool (*accepts)(const std::string&), const char* expected,
	std::string& filepath);

// The editor's part of a frame, before the renderer draws it (see OnUpdate): the camera entity and the sun follow the
// editor's camera and the environment, and what the panels asked for in the last frame happens now, while the renderer
// records nothing: models removed and loaded, pasting, New and Open, material files loaded, maps assigned, materials deleted.
void EditorPreFrame(H2M::CameraH2M* camera, float frameTime);

void ExtractEnvironment();

// The scene's lights into s_Lights (in the order of the hierarchy, their entities in s_LightEntities), and the selected
// light: derived from the selected entity, as a kind and an index into s_Lights' lists, for the code that works with them
void ExtractLights();

// The water is an entity with a WaterComponent: its settings, and its placement as the entity's transform (see
// EnvMapVulkanSceneData::ReadWater). The editing code (Water panel, gizmo) edits s_WaterSettings and writes it back with CommitWater.
void ExtractWater();

// The probe volume, handled like the water (see EnvMapVulkanSceneData::ReadProbeVolume): the Inspector and the
// gizmo edit s_ProbeVolumeSettings and write it back with CommitProbeVolume
void ExtractProbeVolume();

void ShowNotification(const std::string& text, bool isError = false);

// ---- Play mode (EditorPlayMode.cpp)

// The game runs (Play or Simulate), also while paused: the panels show and edit the running game, not the edited scene
bool IsPlaying();

// Simulate (3.7): the game runs with physics only (no game logic: the input test, the scripts), seen through the
// editor's camera, and the viewport keeps the editor's tools
bool IsSimulating();

// Play (not Simulate) runs and isn't paused: the view is the scene's camera, and the viewport is the game's
bool IsGameRunning();

// Play or Simulate at the start of the next frame (the toolbar, --play, --simulate)
void RequestPlay();
void RequestSimulate();

// At the start of a frame (EditorPreFrame): Play, Step and Stop asked for in the last frame; the game's frame
void UpdatePlayMode(H2M::CameraH2M* camera, float frameTime);

// Ctrl+P: Play, Ctrl+Shift+P: Simulate, and either Stops while the game runs (as in Unity). Not while typing in a field,
// and not while a question is open
void HandlePlayModeShortcutKeys();

// The Viewport panel, each frame: it has the focus (the game gets input in the next frame); Escape (Shift+F1 when the game
// binds Escape) takes the focus away, giving the mouse back to the editor
void UpdateGameViewportFocus(bool viewportFocused);

// The game had the viewport's focus in the last frame (its input); the viewport gets the focus in the next frame
bool IsGameViewportFocused();
void FocusViewportNextFrame();

// The running game's input, for the Input panel (nullptr: no game, or Simulate: it has none)
const InputSystem* GetGameInput();

// The running game's physics, for the Inspector and the Physics panel (nullptr: no game)
ScenePhysics* GetGamePhysics();
// The running game's UI (Play, not Simulate; nullptr otherwise): its selected button, the element under the mouse
const SceneUI* GetGameUI();
// The mouse over the Viewport panel's image (view pixels from its top-left corner; inside: the image is hovered) and the
// image's size, each frame: the game's UI reads it (Play)
void SetGamePointer(bool inside, const glm::vec2& position, const glm::vec2& viewSize);

// The running game's sounds, for the Inspector and the Audio panel (nullptr: no game, or Simulate: no sound)
SceneAudio* GetGameAudio();
// The running game's scripts, for the Inspector (nullptr: no game, or Simulate: no scripts run)
ScriptRuntime* GetGameScripts();
// The running game's animators, for the Inspector and the Animator panel (nullptr: no game; Simulate has them too)
SceneAnimation* GetGameAnimation();

// Hot reload (EditorScriptReload.cpp): each frame, after Play and Stop, a newer build of the project's scripts is loaded
// (while the game doesn't run). RequestScriptReload: load the scripts again now (the Project panel's Reload)
void UpdateScriptReload();
void RequestScriptReload();

// The reloads since the editor started, for the Scripts panel: when, how long, and how it went
struct ScriptReloadRecord
{
	char Time[12] = {};          // the clock's time, "14:03:27"
	bool Loaded = false;         // else the log says why it couldn't be
	bool Leaked = false;         // the old assembly is still held
	bool Rebuilt = false;        // a new build (else Reload in the Project panel)
	double Milliseconds = 0.0;
	size_t ClassCount = 0;
};
const std::vector<ScriptReloadRecord>& GetScriptReloadHistory();

// ---- Writing scripts (EditorScriptBuild.cpp, EditorCodeEditor.cpp, 4.7)

// The open project's script project (.csproj), absolute; empty: none
std::string GetScriptProjectPath();

// The scripts are built with dotnet build on a thread of their own: when asked (Build Scripts, Ctrl+B) and, with Build on
// Save, when a script in the script project's folder changes. Errors and warnings go to the log (and the Console), the
// built assembly is loaded by hot reload. UpdateScriptBuild: each frame
struct ScriptBuildResult
{
	bool Finished = false;     // a build has finished since the editor started
	bool Succeeded = false;
	int Errors = 0, Warnings = 0;
	double Seconds = 0.0;
	std::string FirstMessage;  // the first error (or why the build couldn't start)
};
extern bool s_BuildScriptsOnSave;
void RequestScriptBuild();
void UpdateScriptBuild();
bool IsScriptBuildRunning();
ScriptBuildResult GetLastScriptBuild();
void ShutdownScriptBuild(); // waits for a build in progress

// The code editor the scripts open in; Automatic: the first one found of Visual Studio (Windows), VS Code and Rider
enum class CodeEditorKind { Automatic, VisualStudio, VSCode, Rider, System, Count };
extern CodeEditorKind s_CodeEditor;
const char* GetCodeEditorName(CodeEditorKind kind);
const char* GetResolvedCodeEditorName(); // the one Automatic (or a choice that isn't installed) uses
bool IsCodeEditorAvailable(CodeEditorKind kind);
bool OpenScriptProjectInCodeEditor();
// A file at a line (a path relative to the project folder, as the build writes it, or absolute)
bool OpenScriptFile(const std::string& file, int line);
// The .cs file and line declaring a script class ("Sample.Spinner"); false when none is found
bool FindScriptClassFile(const std::string& className, std::string& file, int& line);
// Visual Studio (EditorVisualStudio.cpp, Windows): the file at the line (empty: only the window) in the Visual Studio that
// has the .csproj open, else in one started with it; on a thread of its own. UpdateVisualStudio: each frame, shows the
// result when it went wrong
bool OpenInVisualStudio(const std::string& devenv, const std::string& csproj, const std::string& file, int line);
void UpdateVisualStudio();
// A new script <name>.cs in the scripts' folder, from the template, opened in the code editor and built; className: with
// its namespace (it can be chosen once built). False (error says why) when it can't be made
bool NewScript(const std::string& name, std::string& className, std::string& error);
// New Script asks for its name (the Scripts menu, the Project panel and the Inspector); with an entity, its Script becomes
// the new class. The dialog: each frame, at the top level
void OpenNewScriptDialog(EnvMapVulkanEntityID entity = NoEntity);
void OnImGuiRenderNewScriptDialog();
// The code editor and Build on Save are kept in imgui.ini: the handler is registered at startup, a change marks it to save
void RegisterEditorSettings();
void SaveEditorSettings();

// ---- The Console panel (EditorConsolePanel.cpp)

// The log in the editor (View > Console): filters, a message opened at its file and line (double-click)
extern bool s_ShowConsolePanel;
void OnImGuiRenderConsolePanel();
void ClearConsole();
void ClearConsoleIfAskedOnPlay(); // Play: Clear on Play

// ---- The Scripts panel (EditorScriptsPanel.cpp)

// The scripts' debug view (View > Scripts): the loaded assembly and its classes, the builds and reloads, and while the game
// runs each script instance with its exceptions
extern bool s_ShowScriptsPanel;
void OnImGuiRenderScriptsPanel();

// The OS window the current ImGui window is in: the main window, or its own when it was dragged out (multi-viewports).
// Keys and the mouse are read from it
GLFWwindow* GetCurrentPlatformWindow();

// A game still running at exit is stopped (the edited scene comes back and is checked)
void ShutdownPlayMode();

// Refuses what can't be done while the game runs (Save, New, Open...), with a notification: true when refused
bool RefusedWhilePlaying(const char* what);

// The view options the renderer draws with: the editor's, or while the game runs the game's (no grid, no selection)
const EnvMapVulkanViewOptions* GetDrawnViewOptions();

// The debug overlay to the renderer, each frame: shown or not, with the game's numbers while Play or Simulate runs
void UpdateDebugOverlay();

// Play, Pause and Step, centered in the menu bar (as in Unity), and the game's frame and time while it runs
void OnImGuiRenderPlayToolbar();

// The frame padding that makes the menu bar tall enough for the toolbar's buttons (given while the window with the menu bar
// begins, and while its menu bar begins, so the menus' text is centered in it)
ImVec2 GetMenuBarFramePadding();

// A frame around the viewport's image while the game runs (paused: in another color)
void DrawPlayModeOutline(const ImVec2& imageMin, const ImVec2& imageSize);

// ---- Scene edits (EditorSceneEdits.cpp)

// The model of the selection (the selected model, or the model of the selected part); NoEntity when no model is selected
EnvMapVulkanEntityID GetSelectedModel();

// The selected part's mesh index in its model; -1 when no part is selected
int GetSelectedMeshIndex();

// The part of a model's mesh, or the model itself (meshIndex < 0, or no part for that mesh)
EnvMapVulkanEntityID GetModelOrPart(EnvMapVulkanEntityID model, int meshIndex);

// A map chosen in the editor (Load button, a file dropped): the environment entity asks for it
void RequestEnvironmentMap(const std::string& filepath);

// The scene's environment entity: the default settings, with the map loaded now
void CreateEnvironmentEntity();

// The editor's view (6.3, EditorSceneEdits.cpp): where the editor's camera is and looks, and its field of view; Set
// shows another one from the next frame (an opened scene's, the one from before Play)
EnvMapVulkanSceneSerializer::EditorView GetEditorView();
void SetEditorView(const EnvMapVulkanSceneSerializer::EditorView& view);
// A game camera where the editor looks from (a new scene's, Add > Camera), primary when the scene had none
EnvMapVulkanEntityID CreateCameraAtView(const std::string& name = "Camera");
// Each frame in Edit and Simulate: the view to show and the editor's lens
void UpdateEditorCamera(H2M::CameraH2M* camera);

// Sets a part's transform to an exact matrix (the gizmo's result, Reset Mesh): the mesh gets the matrix as it is, the
// part's transform its decomposition
void SetPartMatrix(EnvMapVulkanEntityID partEntity, const glm::mat4& matrix);

// Loads a model file and adds it to the scene (called at the start of a frame, see EditorPreFrame), and selects it. With a ground position (a model dropped
// on the viewport), the model stands on that point: the bottom center of its bounding box is placed there, rather than its origin.
// A model far too large or too small for the view (e.g. modeled in millimeters) is also scaled to fit (see below).
void LoadModel(const std::string& filepath, std::optional<glm::vec3> groundPosition = std::nullopt);

// Writes an edited copy of a light of s_Lights back into its entity (before: the light as it was read)
void CommitPointLight(int index, const EnvMapVulkanPointLight& before);

void CommitSpotLight(int index, const EnvMapVulkanSpotLight& before);

void CommitSun(const EnvMapVulkanDirectionalLight& before);

// A new point or spot light entity from a light struct (EnvMapVulkanLightEnvironment::Add*Light makes one with its defaults)
EnvMapVulkanEntityID CreatePointLightEntity(const EnvMapVulkanPointLight& light);

EnvMapVulkanEntityID CreateSpotLightEntity(const EnvMapVulkanSpotLight& light);

// The scene's sun (one per scene), with the defaults of EnvMapVulkanDirectionalLight
void CreateSunEntity();

void SyncSunWithEnvironmentRotation();

// Writes an edited s_WaterSettings back into the water entity (before: as it was read); the placement only when it changed
void CommitWater(const EnvMapVulkanWaterSettings& before);

// Adds the water (one per scene) with these settings and selects it
void CreateWaterEntity(const EnvMapVulkanWaterSettings& settings);

void RemoveWaterEntity();

// Writes an edited s_ProbeVolumeSettings back into the entity (before: as it was read); the placement only when it changed
void CommitProbeVolume(const EnvMapVulkanProbeVolumeSettings& before);

void RemoveProbeVolumeEntity();

// The box around every mesh of every model of the scene, in the world (false: the scene has no models)
bool GetSceneModelBounds(glm::vec3& boundsMin, glm::vec3& boundsMax);

// Selects a light of s_Lights; None deselects the selected light (a selected model stays selected)
void SelectLight(LightKind kind, int index = 0);

void SelectWater();

// Called once per frame, before the viewport's gizmos and picking: a selection that no longer exists (a removed entity)
// is cleared, and the lights and the water are read again, so the selected light's kind and index and the water's
// selection follow the selected entity
void SyncLightSelection();

// Points the sun at the brightest light source of the environment map (the sun of an outdoor HDR, a bright window
// indoors) and gives it that light's color. The intensity stays as it is.
void AlignSunToEnvironment();

// Where a new light goes: where the camera looks, so the light is within reach of its default range:
// - the model surface at the center of the viewport
// - else the ground (y = 0) there, when it's not too far
// - else (the camera looks over the scene, e.g. horizontally) the point of the view ray nearest the models' center,
//   at the models' height
// The range of a new point or spot light at this position: far enough to reach every loaded model (the distance to the
// farthest corner of their bounds), at least 10, at most 1000 (the Range drag's limit). Without models: 10.
float GetDefaultLightRange(const glm::vec3& position);

glm::vec3 GetLightSpawnTarget();

EntityKind GetEntityKind(EnvMapVulkanEntityID entity);

const char* GetEntityKindName(EntityKind kind);

// The marker drawn in front of an entity's name in the hierarchy
ImU32 GetEntityKindColor(EntityKind kind);

// Entities that can be dragged under another entity, and the ones that can take children. A part stays in its model, the
// environment, the sun and the water stay at the top (the water is always level: a parent could tilt it).
bool CanBeReparented(EntityKind kind);

bool CanHaveChildren(EntityKind kind);
// Whether a child of that kind may go under a parent of that kind (NoEntity's kind: the top level is Other): UI elements
// only in UI (a canvas or another element), and UI takes only UI
bool CanBeChildOf(EntityKind child, EntityKind parent);

bool CanBeDeleted(EntityKind kind);

// Removes an entity (and its children): a model or a part at the start of the next frame (its GPU resources may be in use),
// a light or the water right away. A model's last mesh takes the model with it (see EditorPreFrame).
void DeleteEntity(EnvMapVulkanEntityID entity);

// ---- Prefabs (EditorPrefabs.cpp, 6.1): requests done at the start of the next frame (ProcessPrefabOperations)
enum class PrefabOperation { Save, Place, Apply, Revert, Unpack };
// Save: the entity as the file (it becomes an instance); Place: the file as a new instance at position (none: where the
// camera looks); Apply, Revert, Unpack: the instance whose root is the entity
void RequestPrefabOperation(PrefabOperation kind, EnvMapVulkanEntityID entity, const std::string& file = std::string(),
	std::optional<glm::vec3> position = std::nullopt);
void ProcessPrefabOperations();
// After a scene opened: the instances of changed prefabs that didn't change themselves take the prefab's new content
void UpdatePrefabInstancesOfOpenedScene();
bool IsPrefabFile(const std::string& path);
// The root of the instance the entity is in (itself, or a parent with a PrefabComponent); NoEntity: none
EnvMapVulkanEntityID FindPrefabInstanceRoot(EnvMapVulkanEntityID entity);
// The project's Assets/Prefabs (empty: no project); a free file name there for an entity's prefab
std::string GetPrefabsFolder();
std::string GetNewPrefabFile(const std::string& folder, const std::string& entityName);
void SaveAsPrefabWithDialog(EnvMapVulkanEntityID entity);
// The Inspector's Prefab section (EditorInspectorPanel.cpp): the instance's prefab, its state, Apply, Revert, Unpack
void PrefabControls(EnvMapVulkanEntityID entity);
// The Content Browser (EditorContentBrowser.cpp)
void OnImGuiRenderContentBrowser();

// A sound alone (Scene Hierarchy: Add > Sound): an entity with an Audio Source where the camera looks, selected
void AddSoundAtView();

// New lights and the water go where the camera looks (see GetLightSpawnTarget), selected
void AddPointLightAtView();

void AddSpotLightAtView();

void AddWaterAtView();

// The probe volume goes around where the camera looks, standing on that point's height
void AddProbeVolumeAtView();

// ---- The scene's file (EditorDocument.cpp)

void OnImGuiRenderNotification();

// What refers to the scene's entities, after its content was cleared or replaced
void ForgetSceneEntities();

// New and Open, from the File menu or a shortcut (asks about unsaved changes first)
void RequestNewScene();

void RequestOpenScene(const std::string& filepath = "");

// The project (Project::GetOpen) from the File menu: a new one in a folder chosen in a dialog (with a first scene, its start
// scene), one opened from its .mproject file (with its start scene), or none (a new scene). Each replaces the scene, so it
// asks about unsaved changes first.
void RequestNewProject();
void RequestOpenProject();
void RequestCloseProject();

// Writes the open project's file (after an edit in the Project panel)
void SaveProject();

// The folder file dialogs for assets (models, maps, images, materials) start in: the open project's asset folder (empty:
// none, the dialog's own choice)
std::string GetAssetFolderForDialog();

// The question before New or Open drops unsaved changes: Save (then go on), Don't Save, Cancel
void OnImGuiRenderUnsavedChangesPopup();

// The File menu's shortcuts: Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S. Not while typing in a field, and not while a question
// is open (a popup)
void HandleSceneShortcutKeys();

// At the start of a frame (EditorPreFrame): New / Open once confirmed
void ProcessSceneOperations();

// Unsaved changes: the scene's text compared with the saved one, a few times a second
void UpdateSceneUnsavedChanges();

// File menu: Save (Save As for a scene never saved), Save As (asks for the file)
void SaveScene();
// --save-scene: the open scene to its file, at exit (never a dialog)
void SaveSceneOnExit();
void SaveSceneAs();

// ---- Viewport (EditorViewport.cpp)

// World space ray from the camera through a point of the viewport (NDC)
void GetCameraRay(float ndcX, float ndcY, glm::vec3& origin, glm::vec3& direction);

// Finds the model and mesh under the mouse: a ray through the cursor is tested against every mesh's bounding box,
// then its triangles (as in SceneHazelEnvMap); the nearest hit wins. hitModel is NoEntity and hitMesh -1 when nothing is hit.
// ndcX, ndcY: cursor position in normalized device coordinates of the viewport (-1..1, +y up)
// hitPosition (optional): the world position of the hit, when something is hit
void RaycastMesh(float ndcX, float ndcY, EnvMapVulkanEntityID& hitModel, int& hitMesh, glm::vec3* hitPosition = nullptr);

// The point and spot lights under a scaled entity (a model) are scaled with it, by the hierarchy; their Range then too,
// by the uniform scale (see MoveLightWith). Not under a light: its children don't scale with it (its matrix isn't scaled).
void ScaleChildLightRanges(EnvMapVulkanEntityID entity, float scale);

// The Viewport panel: the scene's image, with picking (click: select, Ctrl or Shift + click: add or remove), drops (a material,
// a model file, an .hdr map), the light icons, the gizmo and the frame rate
void OnImGuiRenderViewport();

// F11: the viewport fullscreen and back; Escape leaves fullscreen when the game doesn't have the input (in Play, the first
// Escape gives the mouse back, the second leaves). Not while typing in a field or while a question is open. Before the
// viewport each frame: Escape's first press is still the game's then. F3 shows and hides the debug overlay
void HandleViewportFullscreenKeys();
void SetViewportFullscreen(bool fullscreen);

// ---- Scene Hierarchy (EditorHierarchyPanel.cpp)

// A material dropped onto a part (that part) or a model (all its parts). Dropped onto one of several selected entities,
// it goes to all of them (every selected part, and every part of a selected model), and the selection stays.
void AssignMaterialToDropTarget(EnvMapVulkanEntityID target, H2M::RefH2M<EnvMapVulkanMaterial> material);

// Deletes every selected entity that can be deleted (the sun and the environment stay). Lights and the water go right
// away, models and parts at the start of the next frame (see DeleteEntity).
void DeleteSelectedEntities();

void OnImGuiRenderSceneHierarchy();

// ---- Clipboard (EditorClipboard.cpp)

bool CanBeCopied(EntityKind kind);

// The selected entities that can be copied, each once: one under another selected one is copied with it
void CopySelection();

// Called at the start of a frame (see EditorPreFrame)
void PasteClipboard();

void RequestPaste();

// Ctrl+C and Ctrl+V while the viewport or the Scene Hierarchy has the focus (once a frame; not while typing in a field)
void HandleCopyPasteKeys();

// ---- Inspector (EditorInspectorPanel.cpp)

// The water plane: add / remove it, its place and size, the waves and the look (EnvMapVulkanWaterSettings)
void OnImGuiRenderWater();

// A mesh (a part of a model) that the gizmo and the Mesh Transform fields move on their own: a model with more than one
// part, and not a rigged part of a skinned model (the skeleton places those, see GetMeshTransform)
bool CanManipulateMesh(EnvMapVulkanEntityID modelEntity, int meshIndex);

void OnImGuiRenderModelsAndMeshes();

void OnImGuiRenderInspector();

// The Inspector's Script section (EditorInspectorScript.cpp): the entity's C# script class and its fields
void ScriptPropertiesControls(EnvMapVulkanEntityID entity);
// The Inspector's Audio section (EditorInspectorAudio.cpp, 5.3): the entity's Audio Source and Audio Listener
void AudioControls(EnvMapVulkanEntityID entity);
// The Inspector's Animator section (EditorInspectorAnimator.cpp, 6.4): the model's controller, and while the game runs
// its state, crossfade and parameters
void AnimatorControls(EnvMapVulkanEntityID entity);

// ---- The game UI (EditorUI.cpp, 7.1)

enum class UIEntityPreset { Canvas, Panel, Text, Image, Button };
// A new canvas (at the top level), or a new element under the selected UI entity (else the first canvas, made when there
// is none), selected
EnvMapVulkanEntityID AddUIEntity(UIEntityPreset preset);
// The Inspector's UI section of a canvas or a UI element: the canvas, the rectangle (anchor presets), image, text, button
void UIControls(EnvMapVulkanEntityID entity);
// The view size the UI is laid out for in the editor (the viewport image's, set by the Viewport panel each frame)
void SetUIViewSize(const glm::vec2& size);
// The viewport's mouse in Edit (view pixels from the image's top-left corner): a click on a UI element selects it (toggle:
// adds or removes it) and a drag moves it. True while it has the mouse: the viewport's own picking is skipped
bool HandleViewportUIMouse(const glm::vec2& mouseInView, bool clicked, bool toggle);

// ---- Lights (EditorLightsPanel.cpp)

// The selected light's properties (the sun, a point or a spot light), edited on this frame's copy (s_Lights) and written
// back into its entity at the end. Used by the Lights panel and the Inspector.
void LightPropertiesControls();

// The sun, point lights and spot lights: add, delete, enable and edit
void OnImGuiRenderLights();

// ---- Environment (EditorEnvironmentPanel.cpp)

// The environment's settings: the map (load, or drop an .hdr), the skybox blur, exposure, Extract Sun, tonemapping and the
// rotation of the scene's environment entity. Used by the Environment and the Inspectors.
void EnvironmentControls();

// The Environment panel: the environment map and display, the debug views, the selection overlays, ray tracing, the
// reflections, the ambient occlusion and the bloom
void OnImGuiRenderEnvironment();

// ---- Materials (EditorMaterialPanels.cpp)

bool IsImageFile(const std::string& filepath);

// Loads an image and binds it as one of a material's maps, or removes the map when the request has no file path
// (called at the start of a frame, see EditorPreFrame)
void ApplyMaterialTexture(const PendingMaterialTexture& request);

// Deletes a material from the library (called at the start of a frame, see EditorPreFrame). Meshes that used it get the Default material.
void DeleteMaterial(H2M::RefH2M<EnvMapVulkanMaterial> material);

// "Make Unique": the part gets a new variant of its material. It looks the same and follows the material until one of its
// properties is edited, which then applies to this part only (and to other parts given the variant).
void MakePartMaterialUnique(EnvMapVulkanEntityID partEntity);

// All materials of the scene: create, duplicate, delete, and drag onto a mesh (Models and Meshes panel or viewport) to assign
void OnImGuiRenderMaterialLibrary();

// Edits the selected library material in place: every mesh drawn with it changes (values are push constants of every
// draw; maps are written to the material's descriptor set at the start of the next frame)
void OnImGuiRenderMaterialEditor();

// ---- Renderer Stats (EditorStatsPanel.cpp)

std::string FormatCount(uint64_t value);

void OnImGuiRenderRendererStats();

// ---- The Project panel (EditorProjectPanel.cpp)

// The open project's settings (its name, start scene, asset folder and the game's window), saved as they are edited; without
// a project, New Project and Open Project
void OnImGuiRenderProjectPanel();

// ---- The Input panel (EditorInputPanel.cpp)

// The game's actions with their live values and the bindings that gave them, and the connected gamepads (View > Input)
void OnImGuiRenderInputPanel();

// ---- The Physics panel (EditorPhysicsPanel.cpp)

// The running game's bodies, steps and events, the debug views, a query test from the view's center and the project's
// collision layers (View > Physics)
void OnImGuiRenderPhysicsPanel();
void OnImGuiRenderAudioPanel();

// ---- The Animator panel (EditorAnimatorPanel.cpp, 6.4)

// An animator controller's parameters, states and transitions, edited in place (the running game takes edits at once),
// saved to its .manim file (View > Animator). It shows the selected entity's controller, or the one opened here
extern bool s_ShowAnimatorPanel;
void OnImGuiRenderAnimatorPanel();
// The panel shows (and edits) the controller of this file, and opens
void OpenAnimatorController(const std::string& path);
// A new controller for the model: a state for each of its clips, the first the entry (Assets/Animators/<name>.manim);
// its file, empty when it can't be written (notified)
std::string CreateAnimatorController(const H2M::RefH2M<H2M::ModelH2M>& model, const std::string& name);

// A line the viewport draws over the scene, in the world, in its color (ImGui's)
struct EditorWorldLine
{
	glm::vec3 A, B;
	ImU32 Color;
};
// The physics debug views' lines for this frame: the colliders (the running game's, else the edited scene's), the contacts
// and the query test
void GetPhysicsOverlayLines(std::vector<EditorWorldLine>& lines);
