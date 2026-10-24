// The Inspector, with the models', the water's and the probe volume's controls, and the retired Models and Meshes and Water panels


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
#include "Physics/ScenePhysics.h"

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

// The Inspector's controls of the probe volume
static void ProbeVolumeControls(EnvMapVulkanProbeVolumeSettings& probes)
{
	ImGui::Checkbox("Light the Scene", &probes.Enabled);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("On: inside the box, surfaces get their indirect diffuse light from the probes around them.\n"
			"Off: from the environment map, as without a probe volume.\n"
			"The probes keep capturing the scene either way. Until they have (and after Reset to Environment) they hold\n"
			"the environment's light, and both look the same.");
	}

	ImGui::DragFloat3("Center", &probes.Center.x, 0.05f);
	ImGui::DragFloat3("Size", &probes.Size.x, 0.05f, 0.1f, 1000.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The box the probes fill: the first and the last probe of each axis are on its sides.\n"
			"The gizmo moves and sizes it too (it can't be turned). While the volume is selected, the box is outlined.\n"
			"Only surfaces inside the box are fully lit by the probes: just outside it the environment's light fades in\n"
			"(over half a probe spacing). So a room's walls, floor and ceiling belong inside the box: see Fit to Scene.");
	}
	glm::vec3 sceneMin, sceneMax;
	const bool hasModels = GetSceneModelBounds(sceneMin, sceneMax);
	ImGui::BeginDisabled(!hasModels);
	if (ImGui::Button("Fit to Scene"))
	{
		// Around all models, a little larger: the outermost probes then stand just outside the surfaces, not in them
		const glm::vec3 extent = sceneMax - sceneMin;
		probes.Center = (sceneMin + sceneMax) * 0.5f;
		probes.Size = extent * 1.05f + glm::vec3(0.2f);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Sets the center and the size so the box holds every model of the scene, with a small margin.\n"
			"The number of probes stays: in a larger box they stand farther apart (raise Probes to keep them close).");
	}
	ImGui::DragInt3("Probes", &probes.Counts.x, 0.1f, EnvMapVulkanProbeVolumeSettings::MinCount, 32, "%d", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Probes along X, Y and Z (up to %d, %d, %d). More probes follow the light's changes more closely,\n"
			"and cost more memory and more time to update.",
			EnvMapVulkanProbeVolumeSettings::MaxCounts.x, EnvMapVulkanProbeVolumeSettings::MaxCounts.y, EnvMapVulkanProbeVolumeSettings::MaxCounts.z);
	}
	const glm::ivec3 counts = probes.GetCounts();
	const glm::vec3 spacing = probes.GetSpacing();
	const int tilesX = counts.x * counts.y;
	ImGui::TextDisabled("%d probes, %.2f x %.2f x %.2f m apart", counts.x * counts.y * counts.z, spacing.x, spacing.y, spacing.z);
	ImGui::TextDisabled("Atlases: irradiance %d x %d, visibility %d x %d", tilesX * (EnvMapVulkanProbes::IrradianceTexels + 2),
		counts.z * (EnvMapVulkanProbes::IrradianceTexels + 2), tilesX * (EnvMapVulkanProbes::VisibilityTexels + 2),
		counts.z * (EnvMapVulkanProbes::VisibilityTexels + 2));

	ImGui::Separator();
	ImGui::SliderFloat("Normal Bias", &probes.NormalBias, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How far from a surface its probe lookup starts, along the surface's normal (fractions of the\n"
			"smallest probe spacing). Keeps a wall from being judged by the probes behind it.\n"
			"Too little (0): light from behind a wall shows on it. Too much (0.5): the lookup of a surface facing a near\n"
			"one (the underside of a table) starts beyond that one, and gets blotches. 0.25 suits both.");
	}
	ImGui::SliderFloat("View Bias", &probes.ViewBias, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The same, towards the camera. This one takes a lookup away from both walls of a corner, where the light\n"
			"from outside a room leaks in first (bright lines along the room's edges): 0.5 removes most of it.\n"
			"What is left is a matter of the scene: a wall should be at least a tenth of the probe spacing thick,\n"
			"and the probes closer together than the rooms are wide.");
	}
	ImGui::Checkbox("Occlude Reflections", &probes.OccludeReflections);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reflections show the environment (the sky), also where the scene hides it: inside a room.\n"
			"On: a reflection is darkened by what the probes know. They hold the light that really arrives from the\n"
			"reflection's side; the environment map holds what would arrive with nothing in the way. The reflection is\n"
			"multiplied by their ratio (by brightness, never above 1): a closed room stops reflecting the sky.\n"
			"It only darkens: the room itself doesn't appear in the reflection (that needs ray tracing).\n"
			"Debug View > Reflection Occlusion shows the factor.");
	}

	// Capturing the scene into the probes
	ImGui::Separator();
	const EnvMapVulkanProbes::UpdateStatus status = EnvMapVulkanRenderer::GetProbeStatus();
	ImGui::BeginDisabled(!status.Supported);
	ImGui::Checkbox("Update Automatically", &probes.AutoUpdate);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("On: whenever something changes (a light, a mesh, a material, the environment, the volume), the probes\n"
			"capture the scene again, a few per frame, until the change has arrived in all of them. Then the updates stop.\n"
			"Off: the probes keep what they hold until Bake.");
	}
	if (status.Traced)
	{
		// With ray tracing: its own setting, with a much wider range (the two ways cost very differently)
		ImGui::SliderInt("Probes per Frame", &probes.TracedProbesPerFrame, EnvMapVulkanProbeVolumeSettings::MinTracedProbesPerFrame,
			EnvMapVulkanProbeVolumeSettings::MaxTracedProbesPerFrame, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Ray traced: probes updated per frame while they follow a change (at most all of them: %u here).\n"
				"A probe costs its rays, whatever the number of meshes, so a whole volume usually fits into one frame and\n"
				"the light follows a change within a second. Lower it when Probe Update in Renderer Stats costs too much.\n"
				"Without ray tracing (Environment panel > Ray Tracing) the probes draw cube maps instead, with a setting\n"
				"of their own (1 to %d per frame).", EnvMapVulkanRenderer::GetProbeCount(), EnvMapVulkanProbeVolumeSettings::MaxProbesPerFrame);
		}
	}
	else
	{
		ImGui::SliderInt("Probes per Frame", &probes.ProbesPerFrame, 1, EnvMapVulkanProbeVolumeSettings::MaxProbesPerFrame, "%d", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Probes updated per frame while they follow a change. Each one draws the scene around it 6 times (a small\n"
				"cube map), so more probes follow a change faster and cost more frame time: see Probe Update in Rendering Stats.\n"
				"With ray tracing on, the probes trace rays instead and many more fit into a frame.");
		}
	}
	ImGui::SliderInt("Rays per Probe", &probes.RaysPerProbe, EnvMapVulkanProbeVolumeSettings::MinRaysPerProbe,
		EnvMapVulkanProbeVolumeSettings::MaxRaysPerProbe, "%d", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Directions a probe looks in per update (turned differently every update). More rays: less noise in the\n"
			"probes' light, a little more compute time.");
	}
	ImGui::SliderFloat("Hysteresis", &probes.Hysteresis, 0.0f, 0.99f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How much of its old light a probe keeps in an update, once it has followed a change. Higher: calmer light\n"
			"(less noise from the few rays of one update) that takes longer to settle, and follows animated models more slowly.\n"
			"Right after a change the probes keep nothing for %u rounds over all of them, so the new light gets around fast,\n"
			"then the rounds are averaged up to this value (%u more rounds).",
			EnvMapVulkanProbes::PropagationRounds, EnvMapVulkanProbes::GetSettleRounds(probes.GetHysteresis()));
	}
	ImGui::Checkbox("Relocate Probes", &probes.RelocateProbes);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The grid ignores the scene, so some probes land inside walls or objects.\n"
			"On: a probe that sees mostly the backs of surfaces is moved out through the nearest one, and a probe very close\n"
			"to a surface backs away from it, never farther than 45%% of the probe spacing from its place in the grid.\n"
			"A probe with no way out is switched off: nothing is lit by it, and it isn't updated.\n"
			"Off: every probe stands where the grid puts it.\n"
			"Done in the first %u rounds after a mesh or the volume changed; skinned models don't count.\n"
			"Show Probes > State shows the result.",
			EnvMapVulkanProbes::RelocationRounds);
	}
	if (ImGui::Button("Bake"))
	{
		EnvMapVulkanRenderer::BakeProbes();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Updates every probe %u times, as many per frame as possible: the first half lets the light bounce around,\n"
			"the second half is averaged into a calm result. For a scene that doesn't change. The frame rate drops while it runs.",
			EnvMapVulkanProbes::BakeRounds);
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset to Environment"))
	{
		EnvMapVulkanRenderer::ResetProbes();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Every probe back to the environment's light, as if nothing stood around it.\n"
			"With Update Automatically the capture then starts over; without it the probes stay like that until Bake.");
	}
	if (!status.Supported)
	{
		ImGui::TextDisabled("Not available: the GPU doesn't support cube map arrays");
	}
	else if (status.Baking)
	{
		ImGui::ProgressBar(status.BakeProgress, ImVec2(-1.0f, 0.0f), "Baking");
	}
	else if (status.UpdatesLeft > 0)
	{
		ImGui::TextDisabled("%s %u probes per frame, %u frames to go", status.Traced ? "Tracing" : "Updating", status.ProbesThisFrame,
			(status.UpdatesLeft + status.ProbesPerFrame - 1) / status.ProbesPerFrame);
	}
	else if (status.OutOfDate)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Out of date: the scene changed since the last update");
	}
	else
	{
		ImGui::TextDisabled(status.Captured ? "Up to date" : "The probes hold the environment's light");
	}
	if (status.Supported && probes.RelocateProbes)
	{
		ImGui::TextDisabled("%s%u probes moved, %u switched off", status.Relocating ? "Placing the probes: " : "", status.MovedProbes, status.UnusedProbes);
	}
	ImGui::EndDisabled();

	ImGui::Separator();
	ImGui::Checkbox("Show Probes", &probes.ShowProbes);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A ball at every probe, lit by that probe alone: it shows the light the probe holds for each\n"
			"direction, as a white matte ball there would show it");
	}
	ImGui::BeginDisabled(!probes.ShowProbes);
	ImGui::DragFloat("Probe Size", &probes.ProbeRadius, 0.005f, 0.01f, 2.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
	ImGui::Combo("Show", &probes.Show, "Light\0Visibility\0State\0");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("State: green = the probe stands where the grid puts it, yellow = it was moved (out of a mesh, or away\n"
			"from a surface), red = switched off (inside a mesh with no way out). A line joins a moved probe to its\n"
			"place in the grid (drawn through the scene: that place is often inside a wall). See Relocate Probes.\n"
			"Light: the light the probe holds for each direction.\n"
			"Visibility: how far the probe sees in each direction. White: nothing near (as far as a probe stores, 1.5 cells'\n"
			"diagonals). Dark: a surface close by. The probes use it to tell which surfaces they can light, so a wall\n"
			"between a probe and a surface should show as a dark side of the ball.");
	}
	ImGui::EndDisabled();
}

// A row of three drags like ImGui::DragFloat3, with components that can be locked (shown grayed out)
static bool DragFloat3Locked(const char* label, glm::vec3& value, float speed, const bool locked[3], float min = 0.0f, float max = 0.0f,
	const char* lockedTooltip = nullptr)
{
	ImGui::PushID(label);
	const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	const float width = (ImGui::CalcItemWidth() - 2.0f * spacing) / 3.0f;
	bool changed = false;
	for (int i = 0; i < 3; i++)
	{
		if (i > 0)
		{
			ImGui::SameLine(0.0f, spacing);
		}
		ImGui::PushID(i);
		ImGui::BeginDisabled(locked[i]);
		ImGui::SetNextItemWidth(width);
		changed |= ImGui::DragFloat("##value", &value[i], speed, min, max, "%.3f");
		ImGui::EndDisabled();
		if (locked[i] && lockedTooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("%s", lockedTooltip);
		}
		ImGui::PopID();
	}
	ImGui::SameLine(0.0f, spacing);
	ImGui::TextUnformatted(label);
	ImGui::PopID();
	return changed;
}

// The water's transform, as a model's (Translation, Rotation, Scale) with the components it doesn't have locked: it is
// always level (it turns only around Y) and flat (no height scale). Translation is its center and the height of the
// surface; Scale is its size in meters (the water mesh is a 1 m square). Used by the Water and the Models and Meshes panels.
static void WaterTransformControls(EnvMapVulkanWaterSettings& water)
{
	const bool translationLocked[3] = { false, false, false };
	const bool rotationLocked[3] = { true, false, true };
	const bool scaleLocked[3] = { false, true, false };

	glm::vec3 translation(water.Center.x, water.Height, water.Center.y);
	if (DragFloat3Locked("Translation", translation, 0.05f, translationLocked))
	{
		water.Center = glm::vec2(translation.x, translation.z);
		water.Height = translation.y;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The center of the water; Y is the height of the surface");
	}

	glm::vec3 rotation(0.0f, water.Rotation, 0.0f);
	if (DragFloat3Locked("Rotation", rotation, 0.5f, rotationLocked, 0.0f, 0.0f, "Water is always level: it turns only around Y"))
	{
		water.Rotation = std::remainder(rotation.y, 360.0f);
	}

	glm::vec3 scale(water.Size.x, 1.0f, water.Size.y);
	if (DragFloat3Locked("Scale", scale, 0.1f, scaleLocked, 0.1f, 10000.0f, "A flat surface: its height doesn't scale"))
	{
		water.Size = glm::max(glm::vec2(scale.x, scale.z), glm::vec2(0.1f));
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The size of the water in meters, along its own X and Z");
	}
}

// The water's settings: its transform, the waves, the swell, its look, seeing into it and from under it, the caustics and
// the reflection. Edits the given settings (the caller writes them back into the water entity, see CommitWater).
static void WaterSettingsControls(EnvMapVulkanWaterSettings& water)
{
	WaterTransformControls(water);

	ImGui::Separator();
	ImGui::Text("Waves");
	ImGui::Columns(2);
	if (ImGuiWrapper::Property("Direction", water.WaveDirection, 0.5f, 0.0f, 0.0f, PropertyFlag::DragProperty)) // no limits, wraps
	{
		water.WaveDirection = std::remainder(water.WaveDirection, 360.0f);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Degrees around the vertical axis: where the waves move");
	}
	ImGuiWrapper::Property("Speed", water.WaveSpeed, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("World units per second (the ripples move a bit slower, in another direction)");
	}
	ImGuiWrapper::Property("Strength", water.WaveStrength, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Steepness of the waves: 0 is a mirror");
	}
	ImGuiWrapper::Property("Wave Size", water.WaveScale1, 0.05f, 0.1f, 100.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("World size of the larger wave pattern");
	}
	ImGuiWrapper::Property("Ripple Size", water.WaveScale2, 0.05f, 0.1f, 100.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("World size of the smaller wave pattern");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Swell");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Swell Height", water.SwellHeight, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters: the most the surface rises above its level. Gerstner waves move the surface's vertices\n"
			"(the waves above only shade it); 0 keeps it flat");
	}
	ImGuiWrapper::Property("Swell Length", water.SwellLength, 0.1f, 1.0f, 200.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters: the wavelength of the longest of the four waves (the others are shorter and cross it).\n"
			"Longer waves travel faster, as on real water");
	}
	ImGuiWrapper::Property("Steepness", water.SwellSteepness, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("0: round, rolling waves; 1: sharp crests and wide troughs (the surface moves toward the crests)");
	}
	ImGuiWrapper::Property("Wireframe", water.Wireframe);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Draws the surface's grid over it, where the waves move it (brighter on the crests)");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Look");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Scatter Color", water.ScatterColor, PropertyFlag::ColorProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The light the water body sends back up: dark blue-green for deep, clear water,\nbrighter and greener or browner for shallow or murky water");
	}
	ImGuiWrapper::Property("Roughness", water.Roughness, 0.01f, 0.5f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Small: a sharp sun highlight and a clear reflection; larger: a wider highlight and a blurred reflection");
	}
	ImGuiWrapper::Property("Reflection", water.ReflectionStrength, 0.0f, 2.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Strength of the reflected environment (1 = physically based)");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Into the Water");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Tint", water.Transmittance, PropertyFlag::ColorProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The color light keeps after Clarity meters of water: what is under the water\nturns this color with depth (water absorbs red first)");
	}
	ImGuiWrapper::Property("Clarity", water.Clarity, 0.01f, 0.05f, 100.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters: larger values let you see deeper into the water");
	}
	ImGuiWrapper::Property("Refraction", water.RefractionStrength, 0.0f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How much the waves bend the view of what is under the water");
	}
	ImGuiWrapper::Property("Edge Softness", water.EdgeSoftness, 0.0f, 2.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters of water over which the surface fades in at the shore (no hard line)");
	}
	ImGuiWrapper::Property("Foam", water.FoamAmount, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Foam along the shore and around objects standing in the water");
	}
	ImGuiWrapper::Property("Foam Width", water.FoamWidth, 0.01f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Meters of water depth that get foam");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Under the Water");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Transparency", water.TransparencyFromBelow, 0.0f, 1.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The surface seen from under the water. 0 is physical: the sky shows only within Snell's window\n"
			"(about 49 degrees around the vertical), outside it the surface mirrors the water below.\n"
			"0.5: the window covers the whole sky (only grazing views are mirrored); 1: the mirror fades too.\n"
			"How far you see through the water itself is its Clarity (Into the Water).");
	}
	ImGui::Columns(1);

	ImGui::Separator();
	ImGui::Text("Caustics");
	ImGui::PushID("Caustics"); // the labels are the widget ids, and the waves have a Strength too
	ImGui::Columns(2);
	ImGuiWrapper::Property("Caustics", water.Caustics);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The waves bend the sun's light as it enters the water and focus it into moving bright lines\n"
			"on what is under the water (computed from the waves themselves, around the camera)");
	}
	ImGui::BeginDisabled(!water.Caustics);
	ImGuiWrapper::Property("Strength", water.CausticsStrength, 0.0f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("1: as the waves focus the light; more exaggerates the pattern, less fades it");
	}
	ImGuiWrapper::Property("Focus Depth", water.CausticsFocus, 0.01f, 0.1f, 20.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Meters under the surface where the pattern is computed: the deeper, the more the rays\n"
			"have gathered (sharper, brighter lines). Shallower than this, the caustics fade in from the surface.");
	}
	ImGuiWrapper::Property("Area", water.CausticsArea, 0.5f, 2.0f, 500.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Meters: the square around the camera that gets caustics (they fade out at its edges).\n"
			"Smaller: finer detail (the same map covers less water)");
	}
	ImGui::EndDisabled();
	ImGui::Columns(1);
	ImGui::PopID();

	ImGui::Separator();
	ImGui::Text("Reflection");
	ImGui::Columns(2);
	ImGuiWrapper::Property("Planar Reflection", water.PlanarReflection);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The scene reflected in the water (drawn a second time, from the camera mirrored in the water);\n"
			"from under the water, the mirror the surface is outside Snell's window.\n"
			"Off: only the environment map is reflected");
	}
	ImGui::Text("Resolution");
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	const char* resolutions[] = { "Full", "Half", "Quarter" };
	int resolution = water.ReflectionDivisor == 1 ? 0 : (water.ReflectionDivisor == 4 ? 2 : 1);
	if (ImGui::Combo("##ReflectionResolution", &resolution, resolutions, IM_ARRAYSIZE(resolutions)))
	{
		water.ReflectionDivisor = resolution == 0 ? 1 : (resolution == 2 ? 4 : 2);
	}
	ImGui::PopItemWidth();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Of the reflection image, relative to the viewport (the waves hide most of the difference)");
	}
	ImGui::NextColumn();
	ImGuiWrapper::Property("Distortion", water.ReflectionDistortion, 0.0f, 3.0f, PropertyFlag::SliderProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How much the waves bend the reflection");
	}
	ImGui::Columns(1);

}

// The water plane: add / remove it, its place and size, the waves and the look (EnvMapVulkanWaterSettings)
void OnImGuiRenderWater()
{
	ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Water");
	// The panel edits s_WaterSettings (this frame's copy of the water entity) and writes it back at the end
	EnvMapVulkanWaterSettings& water = s_WaterSettings;
	const EnvMapVulkanWaterSettings before = s_WaterSettings;

	if (!water.Enabled)
	{
		if (ImGui::Button("Add Water"))
		{
			// Under the center of the view, at the height of the ground plane
			glm::vec3 target = GetLightSpawnTarget();
			EnvMapVulkanWaterSettings settings = water;
			settings.Center = glm::vec2(target.x, target.z);
			CreateWaterEntity(settings);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("One water plane per scene: a rectangle with waves and reflections,\nmoved and resized with the gizmo when selected");
		}
		ImGui::TextDisabled("No water on the scene");
		ImGui::End();
		return;
	}

	if (ImGui::Button("Remove Water"))
	{
		RemoveWaterEntity();
		ImGui::End();
		return;
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(s_WaterSelected);
	if (ImGui::Button("Select"))
	{
		SelectWater();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Selects the water for the gizmo (or click it in the viewport):\n1 move (height and position), 3 resize");
	}

	WaterSettingsControls(water);

	CommitWater(before);
	ImGui::End();
}

// A mesh (a part of a model) that the gizmo and the Mesh Transform fields move on their own: a model with more than one
// part, and not a rigged part of a skinned model (the skeleton places those, see GetMeshTransform)
bool CanManipulateMesh(EnvMapVulkanEntityID modelEntity, int meshIndex)
{
	H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, modelEntity);
	if (!model)
	{
		return false;
	}
	const auto& meshes = model->GetMeshes();
	if (meshIndex < 0 || meshIndex >= (int)meshes.size() || meshes.size() < 2)
	{
		return false;
	}
	return !(model->IsSkinned() && meshes[meshIndex]->IsRigged);
}

// A Scale drag of a model's or a part's transform: the point and spot lights under the entity get their Range scaled by
// the change's uniform scale, as with the gizmo (see ScaleChildLightRanges)
static void ScaleDragScalingChildLights(const char* label, EnvMapVulkanEntityID entity, glm::vec3& scale)
{
	const glm::vec3 before = scale;
	if (ImGui::DragFloat3(label, &scale.x, 0.01f, 0.001f, 1000.0f) && scale != before)
	{
		const float volumeBefore = std::abs(before.x * before.y * before.z);
		if (volumeBefore > 0.0f)
		{
			ScaleChildLightRanges(entity, std::cbrt(std::abs(scale.x * scale.y * scale.z) / volumeBefore));
			ExtractLights();
		}
	}
}

// The input test (InputTestComponent): the model moves with Move, jumps with Jump and turns with Look while the game runs
static void InputTestControls(EnvMapVulkanEntityID model)
{
	ImGui::Separator();
	bool enabled = s_Scene.Has<InputTestComponent>(model);
	if (ImGui::Checkbox("Input Test", &enabled))
	{
		if (enabled)
		{
			s_Scene.Add<InputTestComponent>(model);
		}
		else
		{
			s_Scene.Remove<InputTestComponent>(model);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Play: the model moves with the Move action (WASD, the arrow keys, the left stick; forward is away\n"
			"from the camera), jumps with Jump (Space, A) and turns with Look (the right stick; the mouse when captured).\n"
			"A test of input until scripts (Phase 4); no physics yet, so it goes through walls. View > Input shows the actions.");
	}
	InputTestComponent* test = s_Scene.TryGet<InputTestComponent>(model);
	if (!test)
	{
		return;
	}
	ImGui::DragFloat("Speed", &test->Speed, 0.1f, 0.0f, 50.0f, "%.1f m/s");
	ImGui::DragFloat("Jump Speed", &test->JumpSpeed, 0.1f, 0.0f, 50.0f, "%.1f m/s");
	ImGui::DragFloat("Gravity", &test->Gravity, 0.1f, 0.0f, 100.0f, "%.1f m/s2");
	ImGui::DragFloat("Turn Speed", &test->TurnSpeed, 0.01f, 0.0f, 10.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Degrees per unit of Look: the mouse gives 0.1 degrees per pixel, the right stick 180 degrees per second");
	}
	ImGui::Checkbox("Capture Mouse", &test->CaptureMouse);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The game takes the mouse while it has input: the cursor is hidden and the mouse turns the model.\n"
			"Escape gives the mouse back to the editor; a click on the view gives it to the game again.");
	}
}

// The model's physics (docs/engine/Game_Engine_Runtime_Guide.html, 3.4): its collider (alone: a static body, as a floor or
// a wall) and its rigid body (it moves). The bodies are made when the game starts.
static void PhysicsControls(EnvMapVulkanEntityID model)
{
	const ImVec4 warningColor(1.0f, 0.8f, 0.3f, 1.0f);
	ImGui::Separator();
	ImGui::PushID("Physics");

	bool hasCollider = s_Scene.Has<ColliderComponent>(model);
	if (ImGui::Checkbox("Collider", &hasCollider))
	{
		if (hasCollider)
		{
			ScenePhysics::FitCollider(s_Scene, model, s_Scene.Add<ColliderComponent>(model));
		}
		else
		{
			s_Scene.Remove<ColliderComponent>(model);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The shape the model collides with while the game runs (a box around the model at first).\n"
			"Alone it makes the model static: a floor, a wall, anything that never moves. With a Rigid Body it moves.");
	}
	ImGui::SameLine();
	bool hasBody = s_Scene.Has<RigidBodyComponent>(model);
	if (ImGui::Checkbox("Rigid Body", &hasBody))
	{
		if (hasBody)
		{
			s_Scene.Add<RigidBodyComponent>(model);
			if (!s_Scene.Has<ColliderComponent>(model))
			{
				ScenePhysics::FitCollider(s_Scene, model, s_Scene.Add<ColliderComponent>(model)); // a body needs a shape
			}
		}
		else
		{
			s_Scene.Remove<RigidBodyComponent>(model);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The model moves with physics while the game runs: it falls, is pushed and bounces (Dynamic),\n"
			"or is moved by the game and pushes the others (Kinematic). It needs a Collider (added with it).");
	}
	ImGui::SameLine();
	bool isCharacter = s_Scene.Has<CharacterControllerComponent>(model);
	if (ImGui::Checkbox("Character", &isCharacter))
	{
		if (isCharacter)
		{
			ScenePhysics::FitCharacter(s_Scene, model, s_Scene.Add<CharacterControllerComponent>(model));
		}
		else
		{
			s_Scene.Remove<CharacterControllerComponent>(model);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A player or another walking figure: a standing capsule (fitted around the model at first) that the game\n"
			"moves by asking for a velocity, as the Input Test does. It stops at walls and slides along them, walks up\n"
			"gentle slopes and steps, slides down steep slopes, stands on things and pushes dynamic models.");
	}
	if (IsPlaying() && (hasCollider || hasBody || isCharacter))
	{
		ImGui::TextDisabled("Changes count from the next Play (the bodies are made at Play)");
	}

	RigidBodyComponent* body = s_Scene.TryGet<RigidBodyComponent>(model);
	if (ColliderComponent* collider = s_Scene.TryGet<ColliderComponent>(model))
	{
		using Shape = ColliderComponent::Shape;
		int shape = (int)collider->Type;
		if (ImGui::Combo("Shape", &shape, "Box\0Sphere\0Capsule\0Convex Mesh\0Triangle Mesh\0"))
		{
			collider->Type = (Shape)shape;
			ScenePhysics::FitCollider(s_Scene, model, *collider);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Box, Sphere, Capsule: simple shapes, the fastest. They fit around the model when chosen.\n"
				"Convex Mesh: the model wrapped tightly, as in plastic film (no holes or hollows): for moving models.\n"
				"Triangle Mesh: the model's triangles as they are (a room, a terrain): for static models only.");
		}

		// The model's world scale: the sizes below are in its own space, and the scale is applied to them
		const glm::mat4 world = s_Scene.GetWorldTransform(model);
		const glm::vec3 scale(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])), glm::length(glm::vec3(world[2])));
		const bool round = collider->Type == Shape::Sphere || collider->Type == Shape::Capsule;
		const bool primitive = collider->Type == Shape::Box || round;
		switch (collider->Type)
		{
			case Shape::Box:
				ImGui::DragFloat3("Half Extents", &collider->HalfExtents.x, 0.01f, 0.001f, 1000.0f, "%.3f");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Half the box's size along X, Y and Z, in the model's own space (before its scale)");
				}
				break;
			case Shape::Sphere:
				ImGui::DragFloat("Radius", &collider->Radius, 0.01f, 0.001f, 1000.0f, "%.3f");
				break;
			case Shape::Capsule:
				ImGui::DragFloat("Radius", &collider->Radius, 0.01f, 0.001f, 1000.0f, "%.3f");
				ImGui::DragFloat("Half Height", &collider->HalfHeight, 0.01f, 0.0f, 1000.0f, "%.3f");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Half the height of its straight middle, along Y: with the round ends it is 2 x (Half Height + Radius) tall");
				}
				break;
			default:
				ImGui::TextDisabled("The model's meshes, at its scale");
				break;
		}
		if (primitive)
		{
			ImGui::DragFloat3("Offset", &collider->Offset.x, 0.01f);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The shape's center, in the model's own space");
			}
			if (ImGui::Button("Fit to Model"))
			{
				ScenePhysics::FitCollider(s_Scene, model, *collider);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The size and the center that fit around the model's meshes");
			}
		}
		if (round && (std::abs(scale.x - scale.y) > 0.01f * scale.x || std::abs(scale.x - scale.z) > 0.01f * scale.x))
		{
			ImGui::TextColored(warningColor, "The model's scale differs per axis: the %s takes the largest\n(it stays round)",
				collider->Type == Shape::Sphere ? "sphere" : "capsule's width");
		}
		if (collider->Type == Shape::TriangleMesh && body && body->Type != RigidBodyComponent::BodyType::Static)
		{
			ImGui::TextColored(warningColor, "It moves: it gets a Convex Mesh instead\n(a Triangle Mesh only for static models)");
		}

		ImGui::SliderFloat("Friction", &collider->Friction, 0.0f, 1.0f, "%.2f");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("How much it grips what it slides on: 0 ice, 0.5 wood, 1 rubber (two touching models: both count)");
		}
		ImGui::SliderFloat("Bounciness", &collider->Restitution, 0.0f, 1.0f, "%.2f");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("0: it stops where it lands. 1: it bounces back as high as it fell (the bouncier of two touching models counts)");
		}
		ImGui::Checkbox("Trigger", &collider->IsTrigger);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("A sensor: things pass through it. It will tell the game what enters and leaves it (Phase 3.6)");
		}
	}

	if (body)
	{
		ImGui::Spacing();
		int type = (int)body->Type;
		ImGui::Combo("Body Type", &type, "Static\0Dynamic\0Kinematic\0");
		body->Type = (RigidBodyComponent::BodyType)type;
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Dynamic: physics moves it (gravity, collisions).\n"
				"Kinematic: the game moves it (as the Input Test does): it pushes dynamic models and isn't pushed back.\n"
				"Static: it never moves (the same as a Collider without a Rigid Body).");
		}
		if (!s_Scene.Has<ColliderComponent>(model))
		{
			ImGui::TextColored(warningColor, "No Collider: it isn't simulated");
		}
		ImGui::BeginDisabled(body->Type != RigidBodyComponent::BodyType::Dynamic);
		ImGui::DragFloat("Mass", &body->Mass, 0.1f, 0.001f, 100000.0f, "%.2f kg", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("A heavier model pushes lighter ones away and is hardly moved by them (it falls just as fast)");
		}
		ImGui::SliderFloat("Linear Damping", &body->LinearDamping, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Angular Damping", &body->AngularDamping, 0.0f, 1.0f, "%.2f");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("How fast moving (Linear) and spinning (Angular) slow down by themselves, as in air or water");
		}
		ImGui::DragFloat("Gravity Factor", &body->GravityFactor, 0.01f, -10.0f, 10.0f, "%.2f");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("1: falls as everything does. 0: floats. Negative: rises");
		}
		ImGui::TextUnformatted("Lock Rotation");
		ImGui::SameLine();
		ImGui::Checkbox("X", &body->LockRotationX);
		ImGui::SameLine();
		ImGui::Checkbox("Y", &body->LockRotationY);
		ImGui::SameLine();
		ImGui::Checkbox("Z", &body->LockRotationZ);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("It doesn't turn around the locked world axes (a player that shouldn't tip over: X and Z)");
		}
		ImGui::Checkbox("Continuous Collision", &body->ContinuousCollision);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("For small, fast models (a bullet): its whole path is checked in each step, so it can't pass\n"
				"through a thin wall between two steps. Costs a little more.");
		}
		ImGui::EndDisabled();
	}

	if (CharacterControllerComponent* character = s_Scene.TryGet<CharacterControllerComponent>(model))
	{
		ImGui::Spacing();
		ImGui::TextUnformatted("Character");
		if (hasCollider || hasBody)
		{
			ImGui::TextColored(warningColor, "Its Collider and Rigid Body aren't used: the capsule is its shape");
		}
		ImGui::DragFloat("Height", &character->Height, 0.01f, 0.02f, 100.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Radius##Character", &character->Radius, 0.01f, 0.01f, 50.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The capsule, in meters in the world (the model's scale doesn't change it): a capsule no taller\n"
				"than twice its radius is a ball");
		}
		ImGui::DragFloat3("Offset##Character", &character->Offset.x, 0.01f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The capsule's center, from the model's position, in meters (it doesn't turn with the model)");
		}
		if (ImGui::Button("Fit to Model##Character"))
		{
			ScenePhysics::FitCharacter(s_Scene, model, *character);
		}
		ImGui::SliderFloat("Max Slope", &character->MaxSlopeDegrees, 0.0f, 89.0f, "%.0f deg");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The steepest ground it can walk up and stand on; on steeper ground it slides down");
		}
		ImGui::DragFloat("Step Height", &character->StepHeight, 0.01f, 0.0f, 2.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The highest step it walks up without jumping (a stair is about 0.18 m, a curb 0.15)");
		}
		ImGui::DragFloat("Mass##Character", &character->Mass, 0.5f, 0.1f, 10000.0f, "%.1f kg", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Its weight presses on what it stands on (a box tips over, a plank bends down)");
		}
		ImGui::DragFloat("Push Strength", &character->PushStrength, 1.0f, 0.0f, 100000.0f, "%.0f N", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The most force it pushes a dynamic model with: 0 pushes nothing, and it is stopped by a box as by a wall");
		}

		// The running game: what the character stands on and how it moves (the editor's copy has none)
		ScenePhysics::CharacterState state;
		if (const ScenePhysics* physics = GetGamePhysics(); physics && physics->GetCharacterState(model, state))
		{
			ImGui::TextDisabled("%s", ScenePhysics::GetGroundName(state.Ground));
			ImGui::TextDisabled("Speed %.2f m/s, %.2f m/s up", glm::length(glm::vec2(state.Velocity.x, state.Velocity.z)), state.Velocity.y);
			if (state.Ground != ScenePhysics::CharacterGround::InAir)
			{
				ImGui::TextDisabled("Ground slope %.0f deg", glm::degrees(std::acos(glm::clamp(state.GroundNormal.y, -1.0f, 1.0f))));
			}
		}
	}
	ImGui::PopID();
}

// A model's properties: its transform, the selected part's transform (when a part of it is selected), its physics, the
// input test and its script, removing the model or the part, the animation (skinned models) and the material slots of its parts. Used by
// the Models and Meshes panel and the Inspector.
static void ModelPropertiesControls(EnvMapVulkanEntityID selectedModel)
{
	const int selectedMeshIndex = GetSelectedModel() == selectedModel ? GetSelectedMeshIndex() : -1;
	H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, selectedModel);
	TransformComponent& entry = s_Scene.Get<TransformComponent>(selectedModel);

	ImGui::Separator();
	ImGui::Text("Transform");
	ImGui::DragFloat3("Translation", &entry.Translation.x, 0.1f);
	ImGui::DragFloat3("Rotation", &entry.Rotation.x, 1.0f);
	ScaleDragScalingChildLights("Scale", selectedModel, entry.Scale);

	// The selected part's place in the model (the viewport gizmo moves it too)
	const EnvMapVulkanEntityID selectedPart = selectedMeshIndex >= 0 ? s_EditorSelection.GetPrimary() : NoEntity;
	if (selectedPart != NoEntity && selectedMeshIndex < (int)model->GetMeshes().size())
	{
		ImGui::Separator();
		ImGui::Text("Mesh Transform: %s", s_Scene.Get<NameComponent>(selectedPart).Name.c_str());
		if (CanManipulateMesh(selectedModel, selectedMeshIndex))
		{
			TransformComponent& partTransform = s_Scene.Get<TransformComponent>(selectedPart);
			ImGui::DragFloat3("Translation##Mesh", &partTransform.Translation.x, 0.1f);
			ImGui::DragFloat3("Rotation##Mesh", &partTransform.Rotation.x, 1.0f);
			ScaleDragScalingChildLights("Scale##Mesh", selectedPart, partTransform.Scale);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Relative to the model. Ctrl + click in the viewport selects the whole model");
			}
			if (ImGui::Button("Reset Mesh"))
			{
				SetPartMatrix(selectedPart, s_Scene.Get<MeshPartComponent>(selectedPart).OriginalTransform);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Back to its place in the model file");
			}
		}
		else
		{
			ImGui::TextDisabled(model->GetMeshes().size() < 2 ? "The model has a single mesh: the Transform above moves it"
				: "A rigged mesh: the skeleton places it (the gizmo moves the whole model)");
		}
	}

	PhysicsControls(selectedModel);
	InputTestControls(selectedModel);
	ScriptPropertiesControls(selectedModel);
	AudioControls(selectedModel);

	ImGui::Separator();
	if (ImGui::Button("Remove Model"))
	{
		s_PendingRemoveEntities.push_back(selectedModel); // removed at the start of the next frame (see EditorPreFrame)
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Removes the whole model, with all its meshes");
	}
	ImGui::SameLine();
	const bool canRemoveMesh = selectedPart != NoEntity && model->GetMeshes().size() > 1;
	ImGui::BeginDisabled(!canRemoveMesh);
	if (ImGui::Button("Remove Mesh"))
	{
		s_PendingRemoveEntities.push_back(selectedPart); // removed at the start of the next frame (see EditorPreFrame)
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(canRemoveMesh ? "Removes only the selected mesh, a part of the model" :
			model->GetMeshes().size() > 1 ? "Select a mesh (a part of the model) below or in the viewport" :
			"The model has a single mesh: use Remove Model");
	}

	// Animation playback (skinned models), like the Animation section of the Mesh Debug panel in SceneHazelEnvMap
	if (model->HasAnimations() && model->IsSkinned())
	{
		ImGui::Separator();
		ImGui::Text("Animation");

		ImGui::Checkbox("Animated", &model->IsAnimated());
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Off: the model is shown in its bind pose");
		}

		ImGui::BeginDisabled(!model->IsAnimated());
		{
			uint32_t animationCount = model->GetAnimationCount();
			if (animationCount > 1)
			{
				if (ImGui::BeginCombo("Clip", model->GetAnimationName(model->GetAnimationIndex()).c_str()))
				{
					for (uint32_t a = 0; a < animationCount; a++)
					{
						ImGui::PushID((int)a);
						if (ImGui::Selectable(model->GetAnimationName(a).c_str(), model->GetAnimationIndex() == a))
						{
							model->SetAnimationIndex(a);
						}
						ImGui::PopID();
					}
					ImGui::EndCombo();
				}
			}
			else
			{
				ImGui::TextDisabled("Clip: %s", model->GetAnimationName(0).c_str());
			}

			if (ImGui::Button(model->AnimationPlaying() ? "Pause" : "Play", ImVec2(60.0f, 0.0f)))
			{
				model->AnimationPlaying() = !model->AnimationPlaying();
			}
			ImGui::SameLine();
			if (ImGui::Button("Restart"))
			{
				model->AnimationTime() = 0.0f;
			}

			// Scrub through the animation (in seconds; ModelH2M keeps the time in animation ticks)
			float ticksPerSecond = model->GetAnimationTicksPerSecond();
			float durationSeconds = model->GetAnimationDuration() / ticksPerSecond;
			float timeSeconds = model->AnimationTime() / ticksPerSecond;
			if (ImGui::SliderFloat("Time", &timeSeconds, 0.0f, durationSeconds, "%.2f s"))
			{
				model->AnimationTime() = timeSeconds * ticksPerSecond;
			}
			ImGui::DragFloat("Time Scale", &model->TimeMultiplier(), 0.01f, 0.0f, 10.0f, "%.2fx");
			ImGui::TextDisabled("Duration %.2f s (%.0f ticks at %.0f/s), %u bones", durationSeconds, model->GetAnimationDuration(), ticksPerSecond, model->GetBoneCount());
		}
		ImGui::EndDisabled();
	}

	// Material slots, one per mesh: select a mesh to edit its material in the Material Editor. The dropdown, or a
	// material dropped from the Material Library, chooses which library material the mesh is drawn with.
	const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();
	const std::vector<EnvMapVulkanEntityID> parts = s_Scene.GetChildren(selectedModel); // a copy: the loop below doesn't change it

	// Assigns a library material to a part; the Material Editor follows if that part is selected
	auto assignMaterial = [&](EnvMapVulkanEntityID partEntity, H2M::RefH2M<EnvMapVulkanMaterial> material) {
		s_Scene.Get<MeshPartComponent>(partEntity).Material = material; // drawn with it from the next frame
		if (partEntity == s_EditorSelection.GetPrimary())
		{
			s_SelectedMaterial = material;
		}
	};
	auto acceptMaterialDrop = [&](EnvMapVulkanEntityID s) {
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload))
			{
				uint32_t index = *(const uint32_t*)payload->Data;
				if (index < materials.size())
				{
					assignMaterial(s, materials[index]);
				}
			}
			ImGui::EndDragDropTarget();
		}
	};

	ImGui::Separator();
	ImGui::Text("Meshes (%d)", (int)model->GetMeshes().size());

	for (EnvMapVulkanEntityID s : parts)
	{
		MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(s);
		if (!part)
		{
			continue;
		}
		ImGui::PushID((void*)(uintptr_t)s);

		const std::string& meshName = s_Scene.Get<NameComponent>(s).Name;
		if (ImGui::Selectable(meshName.c_str(), s_EditorSelection.GetPrimary() == s, 0, ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0.0f)))
		{
			s_EditorSelection.SelectOnly((s_EditorSelection.GetPrimary() == s) ? selectedModel : s); // click again to deselect (back to the whole model)
		}
		acceptMaterialDrop(s);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		const H2M::RefH2M<EnvMapVulkanMaterial> partMaterial = part->Material ? part->Material : EnvMapVulkanMaterialLibrary::GetDefaultMaterial();
		if (ImGui::BeginCombo("##material", partMaterial->GetName().c_str()))
		{
			for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
			{
				ImGui::PushID((int)m);
				if (ImGui::Selectable(materials[m]->GetName().c_str(), partMaterial == materials[m]))
				{
					assignMaterial(s, materials[m]);
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		acceptMaterialDrop(s);

		ImGui::PopID();
	}

	// The selected part's own material: a variant of the one it has, which it can then change without changing other meshes
	if (s_Scene.GetParent(s_EditorSelection.GetPrimary()) == selectedModel && s_Scene.Has<MeshPartComponent>(s_EditorSelection.GetPrimary()))
	{
		const std::string& partName = s_Scene.Get<NameComponent>(s_EditorSelection.GetPrimary()).Name;
		if (ImGui::Button(("Make Unique: '" + partName + "'").c_str()))
		{
			MakePartMaterialUnique(s_EditorSelection.GetPrimary());
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Gives this mesh a variant of its material: it looks the same and follows the material's changes,\n"
				"but the properties you edit in the Material Editor apply to this mesh only");
		}
	}

	if (s_SelectedMaterial)
	{
		std::string label = "Apply '" + s_SelectedMaterial->GetName() + "' to all meshes";
		if (ImGui::Button(label.c_str()))
		{
			for (EnvMapVulkanEntityID partEntity : parts)
			{
				if (MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(partEntity))
				{
					part->Material = s_SelectedMaterial;
				}
			}
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The material shown in the Material Editor");
		}
	}
}

void OnImGuiRenderModelsAndMeshes()
{
	ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Models and Meshes");

	if (ImGui::Button("Load Model"))
	{
		std::string filepath = FileDialogs::OpenFile({}, GetAssetFolderForDialog());
		if (!filepath.empty())
		{
			s_PendingModelFilename = filepath; // loaded at the start of the next frame (see EditorPreFrame)
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("or drop a model here (?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Drag a model file from the Content Browser:\n"
			"- anywhere on this panel: placed at the scene origin\n"
			"- onto the viewport: placed standing on the ground under the cursor");
	}

	ImGui::Separator();

	// The water: listed first (there is at most one). Not a model: no material slots (it has its own shaders, set in the
	// Water panel) and only the transform it has (it is always level and axis-aligned: no rotation, no height scale).
	if (s_WaterSettings.Enabled)
	{
		if (ImGui::Selectable("[Water]", s_WaterSelected))
		{
			SelectWater();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The scene's water plane (its look in the Water panel)");
		}
		if (s_WaterSelected)
		{
			const EnvMapVulkanWaterSettings before = s_WaterSettings;
			ImGui::Separator();
			ImGui::Text("Transform");
			WaterTransformControls(s_WaterSettings);
			CommitWater(before);
			if (ImGui::Button("Remove Water"))
			{
				RemoveWaterEntity();
			}
			ImGui::SameLine();
			if (ImGui::Button("Edit in Water Panel"))
			{
				ImGui::SetWindowFocus("Water");
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Waves, swell, color, caustics and reflection");
			}
		}
		ImGui::Separator();
	}

	// Empty scene: a large box that says where models can be dropped
	const std::vector<EnvMapVulkanEntityID> models = GetModels(s_Scene);
	if (models.empty())
	{
		ImVec2 boxMin = ImGui::GetCursorScreenPos();
		ImVec2 boxSize(ImGui::GetContentRegionAvail().x, glm::max(ImGui::GetContentRegionAvail().y, 80.0f));
		ImVec2 boxMax(boxMin.x + boxSize.x, boxMin.y + boxSize.y);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(boxMin, boxMax, ImGui::GetColorU32(ImGuiCol_TextDisabled), 6.0f, 0, 1.5f);

		const char* hint = "Drop a model here\n(.obj, .fbx, .gltf, .glb, .dae...)";
		ImVec2 textSize = ImGui::CalcTextSize(hint);
		ImGui::SetCursorScreenPos(ImVec2(boxMin.x + (boxSize.x - textSize.x) * 0.5f, boxMin.y + (boxSize.y - textSize.y) * 0.5f));
		ImGui::TextDisabled("%s", hint);
		ImGui::SetCursorScreenPos(boxMin);
		ImGui::Dummy(boxSize);
	}

	const EnvMapVulkanEntityID selectedModel = GetSelectedModel();
	const int selectedMeshIndex = GetSelectedMeshIndex();
	for (EnvMapVulkanEntityID modelEntity : models)
	{
		ImGui::PushID((void*)(uintptr_t)modelEntity);
		const std::string& name = s_Scene.Get<NameComponent>(modelEntity).Name;
		if (ImGui::Selectable(name.c_str(), selectedModel == modelEntity))
		{
			s_EditorSelection.SetPrimary(modelEntity);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", s_Scene.Get<ModelComponent>(modelEntity).FilePath.c_str());
		}
		ImGui::PopID();
	}

	if (selectedModel != NoEntity)
	{
		ModelPropertiesControls(selectedModel);
	}

	// The whole panel is a drop area for model files (loaded at the scene origin)
	ImVec2 panelMin = ImGui::GetWindowPos();
	ImVec2 panelMax(panelMin.x + ImGui::GetWindowSize().x, panelMin.y + ImGui::GetWindowSize().y);
	if (ImGui::BeginDragDropTargetCustom(ImRect(panelMin, panelMax), ImGui::GetID("##ModelsAndMeshesPanelDropArea")))
	{
		std::string filepath;
		if (AcceptFileDrop(panelMin, panelMax, EnvMapVulkanSceneLoader::IsModelFile, "a model file", filepath))
		{
			s_PendingModelFilename = filepath; // loaded at the start of the next frame (see EditorPreFrame)
			s_PendingModelGroundPosition.reset();
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::End();
}

void OnImGuiRenderInspector()
{
	ImGui::SetNextWindowSize(ImVec2(360.0f, 520.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Inspector");

	const EnvMapVulkanEntityID entity = s_EditorSelection.GetPrimary();
	if (!s_Scene.Exists(entity))
	{
		ImGui::TextDisabled("Select an entity in the Scene Hierarchy,\nor click it in the viewport");
		ImGui::End();
		return;
	}
	const EntityKind kind = GetEntityKind(entity);

	// The name (point and spot lights have it with their settings below) and the kind
	if (kind != EntityKind::PointLight && kind != EntityKind::SpotLight)
	{
		char buffer[128] = {};
		snprintf(buffer, sizeof(buffer), "%s", s_Scene.Get<NameComponent>(entity).Name.c_str());
		ImGui::SetNextItemWidth(-90.0f);
		if (ImGui::InputText("##EntityName", buffer, sizeof(buffer)) && buffer[0] != '\0')
		{
			s_Scene.Get<NameComponent>(entity).Name = buffer;
		}
		ImGui::SameLine();
	}
	ImGui::TextDisabled("%s", GetEntityKindName(kind));

	// Tags, for the game's scripts (HasTag, Scene.FindEntitiesWithTag)
	{
		const TagComponent* tags = s_Scene.TryGet<TagComponent>(entity);
		char buffer[256] = {};
		snprintf(buffer, sizeof(buffer), "%s", tags ? tags->ToText().c_str() : "");
		ImGui::SetNextItemWidth(-90.0f);
		if (ImGui::InputTextWithHint("##EntityTags", "Tags, separated by commas", buffer, sizeof(buffer)))
		{
			TagComponent edited;
			edited.FromText(buffer);
			if (edited.Tags.empty())
			{
				if (tags)
				{
					s_Scene.Remove<TagComponent>(entity);
				}
			}
			else
			{
				// The text as typed stays in ImGui's field while it has the focus (a trailing comma, spaces)
				(tags ? s_Scene.Get<TagComponent>(entity) : s_Scene.Add<TagComponent>(entity)) = edited;
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Tags");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Words the game's C# scripts sort entities by: entity.HasTag(\"Pickup\"),\n"
				"Scene.FindEntitiesWithTag(\"Pickup\"). Separated by commas");
		}
	}

	if (const size_t selectedCount = s_EditorSelection.GetAll().size(); selectedCount > 1)
	{
		ImGui::TextDisabled("%zu entities selected: these are the properties of the last one clicked.", selectedCount);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The gizmo moves all selected models, meshes and lights together, Delete removes them,\n"
				"and a material dropped onto one of them goes to all of them");
		}
	}
	ImGui::Separator();

	switch (kind)
	{
		case EntityKind::Model:
			ModelPropertiesControls(entity);
			break;
		case EntityKind::Part:
			ModelPropertiesControls(s_Scene.GetParent(entity));
			break;
		case EntityKind::Sun:
		case EntityKind::PointLight:
		case EntityKind::SpotLight:
			if (kind != EntityKind::Sun)
			{
				if (ImGui::Button("Delete Light"))
				{
					DeleteEntity(entity);
					ImGui::End();
					return;
				}
				ImGui::SameLine();
			}
			ImGui::Checkbox("Shadows Only", &s_ViewOptions.ShadowsOnly);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The viewport shows only the selected light's shadow, whatever the other lights and the exposure:\n"
					"white = lit, black = in shadow, dark gray = the light doesn't reach (out of range or cone, or facing away)");
			}
			LightPropertiesControls();
			break;
		case EntityKind::Water:
		{
			if (ImGui::Button("Remove Water"))
			{
				RemoveWaterEntity();
				ImGui::End();
				return;
			}
			const EnvMapVulkanWaterSettings before = s_WaterSettings;
			WaterSettingsControls(s_WaterSettings);
			CommitWater(before);
			break;
		}
		case EntityKind::ProbeVolume:
		{
			if (ImGui::Button("Remove Probe Volume"))
			{
				RemoveProbeVolumeEntity();
				ImGui::End();
				return;
			}
			const EnvMapVulkanProbeVolumeSettings before = s_ProbeVolumeSettings;
			ProbeVolumeControls(s_ProbeVolumeSettings);
			CommitProbeVolume(before);
			break;
		}
		case EntityKind::Environment:
			EnvironmentControls();
			break;
		case EntityKind::Camera:
		{
			// Edited here, the camera follows in the next frame (see SyncCamera)
			TransformComponent& transform = s_Scene.Get<TransformComponent>(entity);
			ImGui::DragFloat3("Translation", &transform.Translation.x, 0.1f);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Where the camera is. Flying it in the viewport changes these values too.");
			}
			ImGui::DragFloat2("Rotation", &transform.Rotation.x, 1.0f);
			transform.Rotation.x = glm::clamp(transform.Rotation.x, -89.0f, 89.0f);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Where the camera looks, in degrees.\n"
					"X: down (positive) and up, -89 to 89.\n"
					"Y: around the vertical axis: 0 looks along -Z, positive turns to the left.\n"
					"The camera isn't rolled.");
			}
			ImGui::SliderFloat("Field of View", &s_Scene.Get<CameraComponent>(entity).FieldOfView, CameraComponent::MinFieldOfView,
				CameraComponent::MaxFieldOfView, "%.0f deg", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The angle the view spans from its bottom edge to its top edge.\n"
					"Small: a telephoto lens. The view is flat, and things at different distances move almost together\n"
					"when the camera moves.\n"
					"Large: a wide-angle lens. Strong perspective: near things move much more than far ones, and shapes\n"
					"stretch towards the edges. 45 to 60 is usual. Saved with the scene, and an unsaved change.");
			}
			ImGui::TextDisabled("Saved with the scene: an opened scene is seen from here.");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Moving the camera alone doesn't count as an unsaved change: save the scene to keep the view.");
			}
			break;
		}
		case EntityKind::Sound:
		{
			if (ImGui::Button("Delete Sound"))
			{
				DeleteEntity(entity);
				ImGui::End();
				return;
			}
			TransformComponent& transform = s_Scene.Get<TransformComponent>(entity);
			ImGui::DragFloat3("Translation", &transform.Translation.x, 0.1f);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Where the sound is heard from (Spatial), relative to its parent. The gizmo moves it too.");
			}
			break;
		}
		default:
			break;
	}

	// The C# script and the sound of any other entity (a camera that follows the player, and listens); a model's are
	// with its physics above
	if (kind != EntityKind::Environment && kind != EntityKind::Model && kind != EntityKind::Part)
	{
		ScriptPropertiesControls(entity);
		AudioControls(entity);
	}

	ImGui::End();
}
