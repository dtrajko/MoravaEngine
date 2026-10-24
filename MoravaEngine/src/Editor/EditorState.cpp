// The editor's state shared by its files (see EditorState.h)

#include "EditorState.h"

#include "EnvMapVulkan/EnvMapVulkanRenderSettings.h"
#include "EnvMapVulkan/EnvMapVulkanViewOptions.h"


EnvMapVulkanScene s_Scene;
EnvMapVulkanRenderSettings s_RenderSettings;
EnvMapVulkanViewOptions s_ViewOptions = EnvMapVulkanViewOptions::Editor();
H2M::CameraH2M* s_Camera = nullptr;
EditorSelection s_EditorSelection(s_Scene); // see EditorSelection

EnvMapVulkanEntityID s_EnvironmentEntity = NoEntity; // the scene's environment (one per scene)
EnvMapVulkanLightEnvironment s_Lights;
LightEntities s_LightEntities;
LightKind s_SelectedLightKind = LightKind::None;
int s_SelectedLightIndex = 0;
EnvMapVulkanWaterSettings s_WaterSettings;
EnvMapVulkanEntityID s_WaterEntity = NoEntity; // the scene's water (at most one), NoEntity: none
bool s_WaterSelected = false; // the water entity is the selected entity (see ExtractWater)
EnvMapVulkanProbeVolumeSettings s_ProbeVolumeSettings;
EnvMapVulkanEntityID s_ProbeVolumeEntity = NoEntity; // the scene's probe volume (at most one), NoEntity: none

bool s_CameraEntitySynced = false;
glm::vec3 s_SyncedCameraTranslation = glm::vec3(0.0f);
bool s_SunRotationSynced = false;
bool s_PendingSunAlign = true;             // align the sun at the start of the next frame (see EditorPreFrame) (startup, a sun extracted)
bool s_KeepSunOnNextEnvironmentLoad = false; // an opened scene's map: its saved sun is not turned to the map's sun

std::string s_PendingModelFilename;  // requested from the UI, loaded at the start of the next frame (see EditorPreFrame)
std::optional<glm::vec3> s_PendingModelGroundPosition; // dropped on the viewport: where the model is placed (see LoadModel)
std::vector<EnvMapVulkanEntityID> s_PendingRemoveEntities; // models and parts, requested from the UI, removed at the start of the next frame (see EditorPreFrame)
bool s_PendingPaste = false; // requested from the UI, pasted at the start of the next frame (see EditorPreFrame) (models are loaded)
H2M::RefH2M<EnvMapVulkanMaterial> s_PendingDeleteMaterial; // requested from the UI, deleted at the start of the next frame (see EditorPreFrame)
std::vector<PendingMaterialTexture> s_PendingMaterialTextures;
std::vector<std::string> s_PendingMaterialFiles;

std::string s_SceneFilePath;           // empty: never saved ("Untitled")
std::string s_SceneName = "Untitled";
bool s_SceneHasUnsavedChanges = false;

H2M::RefH2M<EnvMapVulkanMaterial> s_SelectedMaterial;      // edited in the Material Editor
EnvMapVulkanEntityID s_RenamingEntity = NoEntity;   // the entity whose name is being edited in the hierarchy
char s_RenameBuffer[128] = {};
bool s_ShowLightGizmos = true; // light icons and shapes in the viewport
bool s_ShowAudioGizmos = true; // sound icons and the selected sound's distances
bool s_ShowRendererStats = true;
bool s_ShowProjectPanel = false; // opened from the File menu (Project Settings), and by New Project
bool s_ShowInputPanel = false;
bool s_ShowPhysicsPanel = false;
bool s_ShowAudioPanel = false;
bool s_ShowPhysicsColliders = false;
bool s_ShowPhysicsContacts = false;
bool s_ShowViewportFps = true;
