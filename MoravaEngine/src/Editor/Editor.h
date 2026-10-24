#pragma once

#include "EnvMapVulkan/EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"
#include "EnvMapVulkan/EnvMapVulkanViewOptions.h"

#include "H2M/Renderer/CameraH2M.h"


/**
 * The scene editor: the scene being edited and its file (New, Open, Save), the panels (Scene Hierarchy, Properties,
 * Environment, Material Library and Editor, Renderer Stats), the viewport with picking, gizmos and light icons, the
 * selection, the clipboard, and Play mode (Play, Pause, Step, Stop: the game runs in the viewport).
 *
 * It owns the scene, how it is drawn (EnvMapVulkanRenderSettings, saved with the scene) and the view's options; the renderer
 * draws them (EnvMapVulkanRenderer::SetScene) and asks the editor to build its windows (SetUserInterface). Every frame:
 * OnUpdate (before the frame is drawn: what the panels asked for happens, the scene changes), then the renderer's Draw,
 * which calls OnImGuiRender while it records the window's image.
 *
 * Written against the renderer's interface only, with ImGui: no graphics API code (see EnvMapVulkanRenderer's "What the
 * editor asks the renderer").
 */
class Editor
{
public:
	// In main, right after Log::Init: the log also goes to the Console panel, from the first line on
	static void InstallConsole();
	// After the renderer's Init: the startup scene (an environment, a sun and a camera); hands the renderer the scene
	static void Init();
	// Before the renderer's Shutdown: the scene and everything the editor holds on to are released
	static void Shutdown();

	// Before the renderer draws the frame. camera: the editor's camera (the viewport's); frameTime: the last frame's time,
	// in seconds (the game's frame, while it runs)
	static void OnUpdate(H2M::CameraH2M* camera, float frameTime);
	// The editor's windows (ImGui), built while the renderer records the window's image
	static void OnImGuiRender();

	// Scene files (.mscene): New and Open ask about unsaved changes and happen at the start of the next frame; Save writes
	// the scene (and its materials with unsaved changes), Save As asks for the file first
	static void NewScene();
	static void OpenScene();
	static void SaveScene();
	static void SaveSceneAs();
};
