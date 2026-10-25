// The editor's frame (Init, Shutdown, OnUpdate, the windows and menus) and what it does before each frame is drawn


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


#include "Editor.h"
#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

static uint32_t s_SeenEnvironmentVersion = 0; // the renderer's environment map the sun was last checked against

// The environment map's rotation (degrees around Y), the scene's environment entity's
float GetEnvironmentRotation()
{
	const EnvironmentComponent* environment = s_Scene.TryGet<EnvironmentComponent>(s_EnvironmentEntity);
	return environment ? environment->Rotation : 0.0f;
}

bool IsEnvironmentMapFile(const std::string& filepath)
{
	std::string extension = std::filesystem::path(filepath).extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return extension == ".hdr";
}

// Accepts a file dragged from the Content Browser over the current drop target, if accepts(path) is true. While dragging, an
// accepted file outlines the drop area (highlightMin..highlightMax) and any other file shows what can be dropped there
// (expected, e.g. "a model file"). Returns true once an accepted file is dropped, with its path in filepath.
bool AcceptFileDrop(const ImVec2& highlightMin, const ImVec2& highlightMax, bool (*accepts)(const std::string&), const char* expected,
	std::string& filepath)
{
	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM",
		ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
	if (!payload)
	{
		return false;
	}

	std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
	if (!accepts(itemPath))
	{
		ImGui::SetTooltip("'%s' can't be dropped here: drop %s", std::filesystem::path(itemPath).filename().string().c_str(), expected);
		return false;
	}

	ImGui::GetForegroundDrawList()->AddRect(highlightMin, highlightMax, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f);
	if (!payload->IsDelivery())
	{
		return false;
	}
	filepath = itemPath;
	return true;
}

void Editor::OnImGuiRender()
{
	{
		// ImGui Dockspace
		bool open = true;
		bool* p_open = &open;

		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background 
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, GetMenuBarFramePadding()); // the menu bar's height: room for the toolbar
		ImGui::Begin("DockSpace Demo", p_open, window_flags);
		ImGui::PopStyleVar();
		{
			if (opt_fullscreen)
			{
				ImGui::PopStyleVar(2);
			}

			// DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}
			else
			{
				// ShowDockingDisabledMessage();
			}

			OnImGuiRenderViewport();


			/**** BEGIN ImGui panels ****/

			// ImGui::Begin("Scene Hierarchy");
			// ImGui::End();

			bool showSceneHierarchyPanel = true;
			// H2M::VulkanTestLayer::s_SceneHierarchyPanel->OnImGuiRender(&showSceneHierarchyPanel);

			// Content Browser (6.1): the project's files (or the engine's), dragged onto the panels; entities dropped on it
			// become prefabs
			OnImGuiRenderContentBrowser();

			bool showMaterialEditorPanel = true;
			// H2M::VulkanTestLayer::s_MaterialEditorPanel->OnImGuiRender(&showMaterialEditorPanel);

			/**** END ImGui panels ****/

			OnImGuiRenderEnvironment();

			// Retired: their functions are in the Scene Hierarchy and the Inspector (the functions are kept, not shown)
			// OnImGuiRenderModelsAndMeshes();
			// OnImGuiRenderLights();
			// OnImGuiRenderWater();
			UpdateSceneUnsavedChanges();
			HandleSceneShortcutKeys();
			HandlePlayModeShortcutKeys();
			OnImGuiRenderUnsavedChangesPopup();
			OnImGuiRenderNotification();
			OnImGuiRenderSceneHierarchy();
			OnImGuiRenderInspector();
			OnImGuiRenderMaterialLibrary();
			OnImGuiRenderMaterialEditor();
			OnImGuiRenderRendererStats();
			OnImGuiRenderProjectPanel();
			OnImGuiRenderInputPanel();
			OnImGuiRenderPhysicsPanel();
			OnImGuiRenderAudioPanel();
			OnImGuiRenderScriptsPanel();
			OnImGuiRenderConsolePanel();
			OnImGuiRenderNewScriptDialog();

			/**** BEGIN DockSpace menu bar ****/

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, GetMenuBarFramePadding()); // the menus' text centered in the taller bar
			const bool menuBar = ImGui::BeginMenuBar();
			ImGui::PopStyleVar();
			if (menuBar)
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					{
						RequestNewScene();
					}
					if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
					{
						RequestOpenScene();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					{
						::SaveScene();
					}
					if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					{
						::SaveSceneAs();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("New Project..."))
					{
						RequestNewProject();
					}
					if (ImGui::MenuItem("Open Project..."))
					{
						RequestOpenProject();
					}
					if (ImGui::MenuItem("Close Project", nullptr, false, Project::GetOpen() != nullptr))
					{
						RequestCloseProject();
					}
					ImGui::MenuItem("Project Settings", nullptr, &s_ShowProjectPanel);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("View"))
				{
					ImGui::MenuItem("Renderer Stats", nullptr, &s_ShowRendererStats);
					ImGui::MenuItem("FPS in the Viewport", nullptr, &s_ShowViewportFps);
					ImGui::MenuItem("Input", nullptr, &s_ShowInputPanel);
					ImGui::MenuItem("Physics", nullptr, &s_ShowPhysicsPanel);
					ImGui::MenuItem("Audio", nullptr, &s_ShowAudioPanel);
					ImGui::MenuItem("Scripts", nullptr, &s_ShowScriptsPanel);
					ImGui::MenuItem("Console", nullptr, &s_ShowConsolePanel);
					ImGui::Separator();
					ImGui::SetNextItemWidth(160.0f);
					ImGui::SliderFloat("Editor Field of View", &s_EditorFieldOfView, CameraComponent::MinFieldOfView,
						CameraComponent::MaxFieldOfView, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("The editor's view's lens (saved with the scene). A game camera has its own (Inspector)");
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Scripts"))
				{
					const bool hasScripts = !GetScriptProjectPath().empty();
					if (ImGui::MenuItem("Build Scripts", "Ctrl+B", false, hasScripts && !IsScriptBuildRunning()))
					{
						RequestScriptBuild();
					}
					if (ImGui::MenuItem("Build on Save", nullptr, &s_BuildScriptsOnSave))
					{
						SaveEditorSettings();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("New Script...", nullptr, false, hasScripts && !IsPlaying()))
					{
						OpenNewScriptDialog();
					}
					if (ImGui::MenuItem("Open in Code Editor", nullptr, false, hasScripts))
					{
						OpenScriptProjectInCodeEditor();
					}
					ImGui::Separator();
					ImGui::MenuItem("Scripts Panel", nullptr, &s_ShowScriptsPanel);
					ImGui::MenuItem("Console", nullptr, &s_ShowConsolePanel);
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Docking"))
				{
					// Disabling fullscreen would allow the window to be moved to the front of other windows,
					// which we can't undo at the moment without finer window depth/z control.
					//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

					// if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
					// if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
					// if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^=  ImGuiDockNodeFlags_NoDockingInCentralNode;
					// if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
					// if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
					ImGui::Separator();
					if (ImGui::MenuItem("Close DockSpace", NULL, false, p_open != NULL))
						*p_open = false;
					ImGui::EndMenu();
				}
				OnImGuiRenderPlayToolbar();
				ImGui::EndMenuBar();
			}
		}
		ImGui::End(); // END DockSpace Demo
		ImGui::PopStyleVar();

		/**** END DockSpace menu bar ****/

	}
}

// The editor's part of a frame, before the renderer draws it (see OnUpdate): the camera entity and the sun follow the
// editor's camera and the environment, and what the panels asked for in the last frame happens now, while the renderer
// records nothing: models removed and loaded, pasting, New and Open, material files loaded, maps assigned, materials deleted.
void EditorPreFrame(H2M::CameraH2M* camera, float frameTime)
{
	// Play, Step and Stop asked for in the last frame, then the game's frame (see EditorPlayMode.cpp)
	UpdatePlayMode(camera, frameTime);

	// The scripts built when one was saved (or asked for), then a newer build loaded while the game doesn't run (after a
	// Stop in this frame, at once)
	UpdateScriptBuild();
	UpdateScriptReload();
	UpdateVisualStudio();

	// The editor's copies of the scene's environment, lights, water and probe volume (see ExtractLights)
	ExtractEnvironment();
	ExtractLights();
	ExtractWater();
	ExtractProbeVolume();
	if (!IsPlaying() || IsSimulating())
	{
		UpdateEditorCamera(camera); // in Play the view is the primary game camera's (UpdatePlayMode); Simulate keeps the editor's
	}
	// The sun turns with the environment map's rotation (EnvMapVulkanDirectionalLight::FollowEnvironmentRotation)
	SyncSunWithEnvironmentRotation();

	// The sun points at the sun of the environment map: at startup, and after loading a map whose sun was taken out of
	// it (its light now has to come from the directional sun), unless the map is an opened scene's (its sun was saved)
	if (EnvMapVulkanRenderer::GetEnvironmentVersion() != s_SeenEnvironmentVersion)
	{
		s_SeenEnvironmentVersion = EnvMapVulkanRenderer::GetEnvironmentVersion();
		if (EnvMapVulkanRenderer::GetExtractedSun().Found && !s_KeepSunOnNextEnvironmentLoad)
		{
			s_PendingSunAlign = true;
		}
		s_KeepSunOnNextEnvironmentLoad = false; // only for the map of the scene that asked for it
	}
	if (s_PendingSunAlign && EnvMapVulkanRenderer::HasEnvironmentMap())
	{
		AlignSunToEnvironment();
		s_PendingSunAlign = false;
	}

	// Set when a change below may leave cached textures unused (a model or mesh removed, a map replaced or removed, a material deleted)
	bool texturesMayBeUnused = false;

	// Model / mesh removal and model loading requested from the Models and Meshes panel
	const std::vector<EnvMapVulkanEntityID> removeEntities = std::move(s_PendingRemoveEntities);
	s_PendingRemoveEntities.clear();
	for (EnvMapVulkanEntityID entity : removeEntities)
	{
		if (!s_Scene.Exists(entity))
		{
			continue; // removed with its model (both were selected), or twice in the list
		}
		// A model's last mesh (its only one, or the last of several removed together): a model without meshes is nothing,
		// so the model goes
		if (s_Scene.Has<MeshPartComponent>(entity))
		{
			const EnvMapVulkanEntityID modelEntity = s_Scene.GetParent(entity);
			H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, modelEntity);
			if (model && model->GetMeshes().size() <= 1 && s_Scene.Has<ModelComponent>(modelEntity))
			{
				Log::GetLogger()->info("Mesh '{0}' was the last one of model '{1}': the model is removed", s_Scene.Get<NameComponent>(entity).Name,
					s_Scene.Get<ModelComponent>(modelEntity).FilePath);
				entity = modelEntity;
			}
		}
		if (const MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(entity))
		{
			if (IsPlaying())
			{
				// Its mesh would leave the model, which the edited scene shares with the game
				ShowNotification("A part of a model can't be removed while the game runs: Stop first", true);
				continue;
			}
			// Only a part of a model: its geometry stays in the model's buffers, so nothing on the GPU is freed. The mesh
			// leaves the model's list, so the parts after it move up by one.
			const EnvMapVulkanEntityID modelEntity = s_Scene.GetParent(entity);
			H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, modelEntity);
			const uint32_t index = part->MeshIndex;
			if (model && index < model->GetMeshes().size() && model->GetMeshes().size() > 1)
			{
				Log::GetLogger()->info("Mesh '{0}' removed from model '{1}'", s_Scene.Get<NameComponent>(entity).Name, s_Scene.Get<ModelComponent>(modelEntity).FilePath);
				model->RemoveMesh(index);
				s_Scene.DestroyEntity(entity);
				for (EnvMapVulkanEntityID sibling : s_Scene.GetChildren(modelEntity))
				{
					MeshPartComponent* other = s_Scene.TryGet<MeshPartComponent>(sibling);
					if (other && other->MeshIndex > index)
					{
						other->MeshIndex--;
					}
				}
				if (s_EditorSelection.GetPrimary() == entity)
				{
					s_EditorSelection.SetPrimary(modelEntity);
				}
				texturesMayBeUnused = true; // its material may have no other users
			}
		}
		else if (s_Scene.Has<ModelComponent>(entity))
		{
			// The model's buffers and descriptor sets may still be used by frames in flight
			EnvMapVulkanRenderer::WaitIdle();
			s_Scene.DestroyEntity(entity);
			s_EditorSelection.Sync(); // the model and its parts leave the selection
			texturesMayBeUnused = true;
		}
	}
	if (!s_PendingModelFilename.empty())
	{
		std::string filepath = s_PendingModelFilename;
		std::optional<glm::vec3> groundPosition = s_PendingModelGroundPosition;
		s_PendingModelFilename.clear();
		s_PendingModelGroundPosition.reset();
		LoadModel(filepath, groundPosition);
	}
	if (s_PendingPaste)
	{
		s_PendingPaste = false;
		PasteClipboard();
	}
	ProcessPrefabOperations(); // saving, placing, Apply, Revert
	ExtractLights(); // after the scene changes above
	ExtractWater();
	ExtractProbeVolume();
	// New / Open from the File menu (an opened scene loads the material files it refers to)
	ProcessSceneOperations();
	// Material files loaded from the Material Library
	if (!s_PendingMaterialFiles.empty())
	{
		EnvMapVulkanRenderer::WaitIdle(); // a material's descriptor set may be in use
		for (const std::string& filepath : s_PendingMaterialFiles)
		{
			if (H2M::RefH2M<EnvMapVulkanMaterial> material = EnvMapVulkanMaterialLibrary::Load(filepath))
			{
				s_SelectedMaterial = material;
				Log::GetLogger()->info("Material '{0}' loaded from '{1}'", material->GetName(), filepath);
			}
		}
		s_PendingMaterialFiles.clear();
		texturesMayBeUnused = true; // replaced maps
	}
	// Maps assigned or removed in the Material Editor, then a material deleted in the Material Library
	for (const PendingMaterialTexture& request : s_PendingMaterialTextures)
	{
		ApplyMaterialTexture(request);
		texturesMayBeUnused = true;
	}
	s_PendingMaterialTextures.clear();
	if (s_PendingDeleteMaterial)
	{
		DeleteMaterial(s_PendingDeleteMaterial);
		s_PendingDeleteMaterial = H2M::RefH2M<EnvMapVulkanMaterial>();
		texturesMayBeUnused = true;
	}
	// Free the GPU memory of textures nothing uses any more. Each change above waited for the device to be idle, and no
	// command buffer of this frame has been recorded yet, so no frame in flight can still sample them.
	if (texturesMayBeUnused)
	{
		uint32_t released = ResourceManager::PurgeUnusedTextures2D();
		if (released > 0)
		{
			Log::GetLogger()->info("{0} unused texture(s) released", released);
		}
	}

}

void ExtractEnvironment()
{
	s_EnvironmentEntity = s_Scene.FindFirst<EnvironmentComponent>();
}

// The scene's lights into s_Lights (in the order of the hierarchy, their entities in s_LightEntities), and the selected
// light: derived from the selected entity, as a kind and an index into s_Lights' lists, for the code that works with them
void ExtractLights()
{
	s_LightEntities = ReadLights(s_Scene, s_Lights);
	s_SelectedLightKind = s_LightEntities.Find(s_EditorSelection.GetPrimary(), s_SelectedLightIndex);
}

// The water is an entity with a WaterComponent: its settings, and its placement as the entity's transform (see
// EnvMapVulkanSceneData::ReadWater). The editing code (Water panel, gizmo) edits s_WaterSettings and writes it back with CommitWater.
void ExtractWater()
{
	s_WaterEntity = ReadWater(s_Scene, s_WaterSettings);
	s_WaterSelected = s_WaterEntity != NoEntity && s_EditorSelection.GetPrimary() == s_WaterEntity;
}

// The probe volume, handled like the water (see EnvMapVulkanSceneData::ReadProbeVolume): the Inspector and the
// gizmo edit s_ProbeVolumeSettings and write it back with CommitProbeVolume
void ExtractProbeVolume()
{
	s_ProbeVolumeEntity = ReadProbeVolume(s_Scene, s_ProbeVolumeSettings);
}

void Editor::Init()
{
	RegisterEditorSettings(); // the code editor and Build on Save, from imgui.ini
	Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE; // as in SceneHazelEnvMap (keys 1/2/3/4 switch the mode)

	// The startup scene: the environment with the map the renderer loaded, a sun and a camera
	CreateEnvironmentEntity();
	CreateSunEntity();
	CreateCameraAtView();
	ExtractEnvironment();
	ExtractLights();
	ExtractWater();
	EnvMapVulkanRenderer::SetScene(&s_Scene, &s_RenderSettings, &s_ViewOptions);
}

void Editor::Shutdown()
{
	ShutdownScriptBuild();
	EnvMapVulkanRenderer::WaitIdle();
	ShutdownPlayMode();
	if (Application::Get()->GetCommandLine().SaveScene)
	{
		SaveSceneOnExit(); // the edited scene, back after a Play
	}
	EnvMapVulkanRenderer::SetScene(nullptr, nullptr, nullptr);
	// The editor's references, released here while the device works: as statics they would be destroyed after main
	// returns, when Vulkan (its validation layer) may already be torn down
	s_Scene.Clear();
	ClearClipboard();
	s_SelectedMaterial = {};
	s_PendingDeleteMaterial = {};
	s_PendingMaterialTextures.clear();
}

void Editor::OnUpdate(H2M::CameraH2M* camera, float frameTime)
{
	s_Camera = camera;
	EditorPreFrame(camera, frameTime);

	// A ray-traced debug view can't stay when the rays were turned off (the switch, a scene opened with it off)
	if (DebugViewNeedsRayTracing(s_ViewOptions.DebugView) && !EnvMapVulkanRenderer::IsRayTracingActive())
	{
		s_ViewOptions.DebugView = DebugViewOff;
		ShowNotification(EnvMapVulkanRenderer::IsRayTracingSupported() ? "The ray-traced views need Ray Tracing turned on (Environment panel)" :
			"The ray-traced views need a GPU with ray tracing", true);
	}

	// The selection as this frame draws it (after the changes above)
	s_ViewOptions.Selection = s_EditorSelection.GetAll();
	s_ViewOptions.PrimarySelection = s_EditorSelection.GetPrimary();
	// The editor's view, or while the game runs the game's
	EnvMapVulkanRenderer::SetScene(&s_Scene, &s_RenderSettings, GetDrawnViewOptions());
}
