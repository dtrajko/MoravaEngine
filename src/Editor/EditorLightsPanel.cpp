// The lights' properties and shadow settings (Inspector), the shadow map viewers, and the retired Lights panel


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

// The shadow map's cascades in the Lights panel ("Show Shadow Map"): the scene seen from the sun, darker = closer to the sun
static bool s_ShowShadowMap = false;
static int s_ShadowMapShownCascade = 0;

static void DrawShadowMapCascades()
{
	const ImTextureID cascadeImage = EnvMapVulkanRenderer::GetShadowCascadeImage((uint32_t)s_ShadowMapShownCascade);
	if (!cascadeImage)
	{
		return;
	}
	if (!EnvMapVulkanRenderer::AreShadowsRendered())
	{
		ImGui::TextDisabled("No shadows this frame (sun off, Cast Shadows off or no models)");
		return;
	}
	// One cascade at a time, as large as the panel allows
	for (int i = 0; i < (int)ShadowCascadeCount; i++)
	{
		if (i > 0)
		{
			ImGui::SameLine();
		}
		char label[32];
		snprintf(label, sizeof(label), "%d##ShadowCascade", i);
		ImGui::RadioButton(label, &s_ShadowMapShownCascade, i);
	}
	const EnvMapVulkanShadowCascade& cascade = EnvMapVulkanRenderer::GetShadowCascade((uint32_t)s_ShadowMapShownCascade);
	ImGui::TextDisabled("Up to %.1f units from the camera, %.1f mm per texel", cascade.SplitDistance, cascade.TexelWorldSize * 1000.0f);
	const float size = glm::max(ImGui::GetContentRegionAvail().x, 16.0f);
	ImGui::Image(EnvMapVulkanRenderer::GetShadowCascadeImage((uint32_t)s_ShadowMapShownCascade), ImVec2(size, size));
}

// An azimuth drag without limits (as Env Map Rotation): it can be dragged endlessly in both directions and wraps around
// to stay in -180..180
static bool AzimuthProperty(float& azimuth)
{
	if (ImGuiWrapper::Property("Azimuth", azimuth, 0.5f, 0.0f, 0.0f, PropertyFlag::DragProperty)) // min = max = 0: no limits
	{
		azimuth = std::remainder(azimuth, 360.0f);
		return true;
	}
	return false;
}

// The sun's shadow settings, in the Lights panel when the sun is selected
static void OnImGuiRenderShadowSettings()
{
	if (!ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	EnvMapVulkanShadowSettings& settings = s_RenderSettings.Shadows;

	ImGui::Columns(2);
	ImGuiWrapper::Property("Distance", settings.Distance, 0.5f, 2.0f, 1000.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Shadows reach this far from the camera (they fade out before it).\nShorter: sharper shadows, as the cascades cover less");
	}

	// Resolution: the shadow map is recreated at the start of the next frame
	static const uint32_t s_Resolutions[] = { 1024, 2048, 4096 };
	static const char* s_ResolutionLabels[] = { "1024 (16 MB)", "2048 (64 MB)", "4096 (256 MB)" };
	int selected = 1;
	for (int i = 0; i < 3; i++)
	{
		if (s_Resolutions[i] == settings.Resolution)
		{
			selected = i;
		}
	}
	ImGui::Text("Resolution");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if (ImGui::BeginCombo("##ShadowResolution", s_ResolutionLabels[selected]))
	{
		for (int i = 0; i < 3; i++)
		{
			if (ImGui::Selectable(s_ResolutionLabels[i], i == selected))
			{
				settings.Resolution = s_Resolutions[i];
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of each of the 4 cascades (GPU memory for all of them)");
	}
	ImGui::PopItemWidth();
	ImGui::NextColumn();

	ImGuiWrapper::Property("Split", settings.SplitLambda, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How the distance is split into cascades:\n0 = equal parts, 1 = short near ones (sharp close to the camera), long far ones");
	}
	ImGuiWrapper::Property("Softness", settings.Softness, 0.0f, 4.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Width of the soft shadow edge (spacing of the 5 x 5 PCF samples, in shadow map texels)");
	}
	ImGuiWrapper::Property("Depth Bias", settings.DepthBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Pushes the shadow map's depth away from the sun: removes \"shadow acne\" (stripes on lit surfaces).\nToo much detaches shadows from the objects (\"peter panning\")");
	}
	ImGuiWrapper::Property("Slope Bias", settings.SlopeBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Depth bias that grows on surfaces at a grazing angle to the sun, where acne appears first");
	}
	ImGuiWrapper::Property("Normal Bias", settings.NormalBias, 0.0f, 5.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Looks the shadow map up this many texels off the surface, along its normal: removes acne\nwithout detaching the shadows as much as the depth bias");
	}
	ImGuiWrapper::Property("Show Cascades", s_ViewOptions.ShowCascades);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Tints the scene by cascade: red, green, blue, yellow (nearest to farthest)");
	}
	ImGui::Columns(1);

	if (ImGui::Button("Reset Shadow Settings"))
	{
		settings = EnvMapVulkanShadowSettings();
	}

	ImGui::Checkbox("Show Shadow Map", &s_ShowShadowMap);
	if (s_ShowShadowMap)
	{
		DrawShadowMapCascades();
	}
}

// "Cast Shadows" of a point or spot light: off when all shadow slots of its kind are taken by other lights
static void LocalCastShadowsProperty(bool& castShadows, uint32_t shadowedCount, uint32_t maxShadowed, bool available, const char* kind)
{
	const bool full = !castShadows && shadowedCount >= maxShadowed;
	ImGui::BeginDisabled(full || !available);
	ImGuiWrapper::Property("Cast Shadows", castShadows);
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		if (!available)
		{
			ImGui::SetTooltip("Not available: the GPU doesn't support cube map arrays (imageCubeArray)");
		}
		else
		{
			ImGui::SetTooltip("%u of %u %s lights cast shadows%s", shadowedCount, maxShadowed, kind, full ? ": all shadow slots are taken" : "");
		}
	}
}

// A resolution combo (the map is recreated at the start of the next frame): labels "<resolution> (<memory>)"
static void ShadowResolutionProperty(const char* label, const uint32_t (&resolutions)[3], const char* const (&labels)[3], uint32_t& resolution)
{
	int selected = 1;
	for (int i = 0; i < 3; i++)
	{
		if (resolutions[i] == resolution)
		{
			selected = i;
		}
	}
	ImGui::Text("%s", label);
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	ImGui::PushID(label);
	if (ImGui::BeginCombo("##Resolution", labels[selected]))
	{
		for (int i = 0; i < 3; i++)
		{
			if (ImGui::Selectable(labels[i], i == selected))
			{
				resolution = resolutions[i];
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();
	ImGui::PopItemWidth();
	ImGui::NextColumn();
}

// The shadow settings of the spot and point lights (shared by all of them), and the selected light's shadow map
static void OnImGuiRenderLocalShadowSettings()
{
	if (!ImGui::CollapsingHeader("Shadows (Spot and Point Lights)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	EnvMapVulkanLocalShadowSettings& settings = s_RenderSettings.LocalShadows;

	ImGui::Columns(2);
	static const uint32_t s_SpotResolutions[3] = { 512, 1024, 2048 };
	static const char* const s_SpotLabels[3] = { "512 (4 MB)", "1024 (16 MB)", "2048 (64 MB)" };
	static const uint32_t s_PointResolutions[3] = { 256, 512, 1024 };
	static const char* const s_PointLabels[3] = { "256 (6 MB)", "512 (24 MB)", "1024 (96 MB)" };
	ShadowResolutionProperty("Spot Resolution", s_SpotResolutions, s_SpotLabels, settings.SpotResolution);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of each spot light's shadow map (GPU memory for all %u)", MaxShadowedSpotLights);
	}
	ShadowResolutionProperty("Point Resolution", s_PointResolutions, s_PointLabels, settings.PointResolution);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of each cube face of a point light's shadow map (GPU memory for all %u cubes)", MaxShadowedPointLights);
	}
	ImGuiWrapper::Property("Softness", settings.Softness, 0.0f, 4.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Width of the soft shadow edge (spacing of the 5 x 5 PCF samples, in shadow map texels)");
	}
	ImGuiWrapper::Property("Depth Bias", settings.DepthBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Removes \"shadow acne\" (stripes on lit surfaces); too much detaches shadows from the objects");
	}
	ImGuiWrapper::Property("Slope Bias", settings.SlopeBias, 0.0f, 10.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Depth bias that grows on surfaces at a grazing angle to the light");
	}
	ImGuiWrapper::Property("Normal Bias", settings.NormalBias, 0.0f, 5.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Looks the shadow map up this many texels off the surface, along its normal");
	}
	ImGui::Columns(1);

	if (ImGui::Button("Reset Shadow Settings##Local"))
	{
		settings = EnvMapVulkanLocalShadowSettings();
	}

	// The selected light's shadow map, drawn by the viewer with the next frame (see EnvMapVulkanShadowMapViewer)
	ImGui::Checkbox("Show Shadow Map", &s_ShowShadowMap);
	if (!s_ShowShadowMap)
	{
		return;
	}
	const bool spot = s_SelectedLightKind == LightKind::Spot;
	const ImTextureID viewerImage = EnvMapVulkanRenderer::ShowLocalShadowMap(s_SelectedLightKind, s_SelectedLightIndex);
	if (!viewerImage)
	{
		ImGui::TextDisabled("No shadow map this frame (Cast Shadows off, light off,\nor all shadow slots taken)");
		return;
	}
	const float width = glm::max(ImGui::GetContentRegionAvail().x, 16.0f);
	ImGui::TextDisabled(spot ? "Seen from the light, darker = closer to it" : "The 6 cube faces: -X +Z +X -Z, +Y above, -Y below; darker = closer");
	if (spot)
	{
		// The spot map is the left square of the viewer image
		ImGui::Image(viewerImage, ImVec2(width, width), ImVec2(0.0f, 0.0f), ImVec2(0.75f, 1.0f));
	}
	else
	{
		ImGui::Image(viewerImage, ImVec2(width, width * 0.75f));
	}
}

static void LightNameProperty(std::string& name)
{
	char buffer[128] = {};
	snprintf(buffer, sizeof(buffer), "%s", name.c_str());
	ImGui::Text("Name");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText("##Name", buffer, sizeof(buffer)))
	{
		name = buffer;
	}
	ImGui::PopItemWidth();
	ImGui::NextColumn();
}

static void LightRangeProperty(float& range)
{
	ImGuiWrapper::Property("Range", range, 0.05f, 0.01f, 1000.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The distance where the light fades out completely");
	}
}

// The selected light's properties (the sun, a point or a spot light), edited on this frame's copy (s_Lights) and written
// back into its entity at the end. Used by the Lights panel and the Inspector.
void LightPropertiesControls()
{
	EnvMapVulkanLightEnvironment before = s_Lights;
	auto commitAll = [&before]() {
		CommitSun(before.Sun);
		for (int i = 0; i < (int)s_Lights.PointLights.size() && i < (int)before.PointLights.size(); i++)
		{
			CommitPointLight(i, before.PointLights[i]);
		}
		for (int i = 0; i < (int)s_Lights.SpotLights.size() && i < (int)before.SpotLights.size(); i++)
		{
			CommitSpotLight(i, before.SpotLights[i]);
		}
	};

	if (s_SelectedLightKind == LightKind::None)
	{
		ImGui::TextDisabled("Select a light in the list,\nor click its icon in the viewport");
		return;
	}

	if (s_SelectedLightKind == LightKind::Sun)
	{
		ImGui::BeginDisabled(!EnvMapVulkanRenderer::HasEnvironmentMap());
		if (ImGui::Button("Align to Environment"))
		{
			commitAll();
			AlignSunToEnvironment();
			before = s_Lights;
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Points the sun at the brightest light of the environment map\n(the sun outdoors, a bright window indoors) and takes its color");
		}
	}

	ImGui::Columns(2);
	if (s_SelectedLightKind == LightKind::Sun)
	{
		EnvMapVulkanDirectionalLight& sun = s_Lights.Sun;
		ImGuiWrapper::Property("Enabled", sun.Enabled);
		ImGuiWrapper::Property("Color", sun.Color, PropertyFlag::ColorProperty);
		ImGuiWrapper::Property("Intensity", sun.Intensity, 0.01f, 0.0f, 20.0f, PropertyFlag::DragProperty);
		AzimuthProperty(sun.Azimuth);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Compass direction of the sun (degrees around the vertical axis, 0 = +Z, 90 = +X)");
		}
		ImGuiWrapper::Property("Elevation", sun.Elevation, 0.5f, -90.0f, 90.0f, PropertyFlag::DragProperty);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Height of the sun above the horizon (degrees, 90 = straight overhead)");
		}
		ImGuiWrapper::Property("Cast Shadows", sun.CastShadows);
		ImGuiWrapper::Property("Follow Env Rotation", sun.FollowEnvironmentRotation);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Env Map Rotation (Environment panel) turns the sun too,\nso a sun aligned to the environment stays on the sun of the map");
		}
	}
	else if (s_SelectedLightKind == LightKind::Point)
	{
		EnvMapVulkanPointLight& light = s_Lights.PointLights[s_SelectedLightIndex];
		LightNameProperty(light.Name);
		ImGuiWrapper::Property("Enabled", light.Enabled);
		ImGuiWrapper::Property("Color", light.Color, PropertyFlag::ColorProperty);
		ImGuiWrapper::Property("Intensity", light.Intensity, 0.1f, 0.0f, 10000.0f, PropertyFlag::DragProperty);
		ImGuiWrapper::Property("Position", light.Position, 0.05f, 0.0f, 0.0f, PropertyFlag::DragProperty);
		LightRangeProperty(light.Range);
		LocalCastShadowsProperty(light.CastShadows, s_Lights.CountShadowedPointLights(), MaxShadowedPointLights, EnvMapVulkanRenderer::CanPointLightsCastShadows(), "point");
	}
	else
	{
		EnvMapVulkanSpotLight& light = s_Lights.SpotLights[s_SelectedLightIndex];
		LightNameProperty(light.Name);
		ImGuiWrapper::Property("Enabled", light.Enabled);
		ImGuiWrapper::Property("Color", light.Color, PropertyFlag::ColorProperty);
		ImGuiWrapper::Property("Intensity", light.Intensity, 0.1f, 0.0f, 10000.0f, PropertyFlag::DragProperty);
		ImGuiWrapper::Property("Position", light.Position, 0.05f, 0.0f, 0.0f, PropertyFlag::DragProperty);
		LightRangeProperty(light.Range);
		AzimuthProperty(light.Azimuth);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Compass direction the spot points to (degrees around the vertical axis, 0 = +Z, 90 = +X)");
		}
		ImGuiWrapper::Property("Elevation", light.Elevation, 0.5f, -90.0f, 90.0f, PropertyFlag::DragProperty);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Tilt of the spot (degrees, -90 = straight down, 0 = horizontal)");
		}
		// The inner cone always stays inside the outer one: dragging either one past the other moves both
		if (ImGuiWrapper::Property("Inner Angle", light.InnerAngle, 0.2f, 0.0f, 89.0f, PropertyFlag::DragProperty))
		{
			light.OuterAngle = std::max(light.OuterAngle, light.InnerAngle);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Half-angle of the fully lit cone (degrees)");
		}
		if (ImGuiWrapper::Property("Outer Angle", light.OuterAngle, 0.2f, 0.1f, 89.0f, PropertyFlag::DragProperty))
		{
			light.InnerAngle = std::min(light.InnerAngle, light.OuterAngle);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Half-angle where the light fades out completely (degrees)");
		}
		LocalCastShadowsProperty(light.CastShadows, s_Lights.CountShadowedSpotLights(), MaxShadowedSpotLights, EnvMapVulkanRenderer::CanSpotLightsCastShadows(), "spot");
		ImGui::Columns(1);
		ImGui::TextDisabled("Viewport gizmo: 1 moves the light, 2 aims it");
	}
	ImGui::Columns(1);

	if (s_SelectedLightKind == LightKind::Sun)
	{
		ImGui::TextDisabled("Viewport gizmo: 1 moves the sun's icon (only the icon:\nthe sun has no position), 2 turns the sun");
		if (ImGui::Button("Icon to View"))
		{
			glm::mat4 cameraTransform = glm::inverse((*s_Camera).GetViewMatrix());
			s_Lights.Sun.IconPosition = glm::vec3(cameraTransform[3]) - glm::normalize(glm::vec3(cameraTransform[2])) * 10.0f;
			s_Lights.Sun.IconMoved = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Puts the sun's icon 10 units in front of the camera (when the sun is behind you)");
		}
		ImGui::SameLine();
		ImGui::BeginDisabled(!s_Lights.Sun.IconMoved);
		if (ImGui::Button("Reset Icon"))
		{
			s_Lights.Sun.IconMoved = false;
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Back in the sky, in the sun's direction");
		}
		OnImGuiRenderShadowSettings();
	}
	else
	{
		OnImGuiRenderLocalShadowSettings();
	}


	commitAll();
}

// The sun, point lights and spot lights: add, delete, enable and edit
void OnImGuiRenderLights()
{
	ImGui::Begin("Lights");

	// The panel edits s_Lights (this frame's copy of the light entities): what changed goes back into the entities at the
	// end. Adding and deleting work on the entities directly, then read them again.
	EnvMapVulkanLightEnvironment before = s_Lights;
	auto commitAll = [&before]() {
		CommitSun(before.Sun);
		for (int i = 0; i < (int)s_Lights.PointLights.size() && i < (int)before.PointLights.size(); i++)
		{
			CommitPointLight(i, before.PointLights[i]);
		}
		for (int i = 0; i < (int)s_Lights.SpotLights.size() && i < (int)before.SpotLights.size(); i++)
		{
			CommitSpotLight(i, before.SpotLights[i]);
		}
	};

	// New lights go above GetLightSpawnTarget(): a point light 1.5 units above it, a spot light 3 units above it, pointing down

	ImGui::BeginDisabled(!s_Lights.CanAddPointLight());
	if (ImGui::Button("Add Point Light"))
	{
		commitAll();
		if (EnvMapVulkanPointLight* light = s_Lights.AddPointLight(GetLightSpawnTarget() + glm::vec3(0.0f, 1.5f, 0.0f), MaxShadowedPointLights))
		{
			light->Range = GetDefaultLightRange(light->Position);
			s_EditorSelection.SetPrimary(CreatePointLightEntity(*light));
		}
		ExtractLights();
		before = s_Lights;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Up to %u point lights", EnvMapVulkanLightsGPU::MaxPointLights);
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!s_Lights.CanAddSpotLight());
	if (ImGui::Button("Add Spot Light"))
	{
		commitAll();
		if (EnvMapVulkanSpotLight* light = s_Lights.AddSpotLight(GetLightSpawnTarget() + glm::vec3(0.0f, 3.0f, 0.0f), MaxShadowedSpotLights))
		{
			light->Range = GetDefaultLightRange(light->Position);
			s_EditorSelection.SetPrimary(CreateSpotLightEntity(*light));
		}
		ExtractLights();
		before = s_Lights;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Up to %u spot lights (pointing down when added)", EnvMapVulkanLightsGPU::MaxSpotLights);
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(s_SelectedLightKind != LightKind::Point && s_SelectedLightKind != LightKind::Spot);
	if (ImGui::Button("Delete"))
	{
		commitAll();
		s_Scene.DestroyEntity(s_LightEntities.Get(s_SelectedLightKind, s_SelectedLightIndex));
		s_EditorSelection.SetPrimary(NoEntity);
		ExtractLights();
		before = s_Lights;
	}
	ImGui::EndDisabled();

	ImGui::Checkbox("Show in Viewport", &s_ShowLightGizmos);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Light icons (click one to select the light) and the selected light's range and cone");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Shadows Only", &s_ViewOptions.ShadowsOnly);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The viewport shows only the selected light's shadow, whatever the other lights and the exposure:\n"
			"white = lit, black = in shadow, dark gray = the light doesn't reach (out of range or cone, or facing away)");
	}

	// The light list: a checkbox (enabled) and a selectable name per light
	auto lightRow = [](const char* name, bool& enabled, LightKind kind, int index) {
		ImGui::PushID((int)kind * 1000 + index);
		ImGui::Checkbox("##Enabled", &enabled);
		ImGui::SameLine();
		if (ImGui::Selectable(name, s_SelectedLightKind == kind && s_SelectedLightIndex == index))
		{
			SelectLight(kind, index);
		}
		ImGui::PopID();
	};

	ImGui::Separator();
	if (ImGui::BeginChild("LightList", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 8.0f), ImGuiChildFlags_Borders))
	{
		lightRow("Sun", s_Lights.Sun.Enabled, LightKind::Sun, 0);
		for (int i = 0; i < (int)s_Lights.PointLights.size(); i++)
		{
			lightRow(s_Lights.PointLights[i].Name.c_str(), s_Lights.PointLights[i].Enabled, LightKind::Point, i);
		}
		for (int i = 0; i < (int)s_Lights.SpotLights.size(); i++)
		{
			lightRow(s_Lights.SpotLights[i].Name.c_str(), s_Lights.SpotLights[i].Enabled, LightKind::Spot, i);
		}
	}
	ImGui::EndChild();

	// Properties of the selected light
	ImGui::Separator();
	commitAll();
	LightPropertiesControls();
	ImGui::End();
}
