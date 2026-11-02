// The Environment panel: the environment map, the debug views, the overlays, ray tracing, reflections, ambient occlusion, bloom


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

// Editor overlays: selection outline, wireframe and bounding boxes (as "Display Outline / Wireframe / Bounding Boxes" in
// SceneHazelEnvMap). They are drawn into two LDR framebuffers, which the viewport composite adds after tonemapping, so they
// keep their colors (no exposure, tonemapping or bloom):
// - overlay: wireframe (depth tested against all meshes, drawn there first) and bounding boxes (always on top)
// - selection mask: silhouette of the selected model or mesh; the composite draws the outline around it
static const char* s_OverlayScopeNames[] = { "Off", "Selected", "All" };

// The environment's settings: the map (load, or drop an .hdr), the skybox blur, exposure, Extract Sun, tonemapping and the
// rotation of the scene's environment entity. Used by the Environment and the Inspectors.
void EnvironmentControls()
{
	EnvironmentComponent* environmentComponent = s_Scene.TryGet<EnvironmentComponent>(s_EnvironmentEntity);
	if (!environmentComponent)
	{
		return;
	}
	EnvironmentComponent& environment = *environmentComponent;
	ImGui::Columns(2);

	// Currently loaded environment map (file name only; the full path is in the tooltip)
	std::string envMapName = std::filesystem::path(EnvMapVulkanRenderer::GetEnvironmentMapFile()).filename().string();
	char envMapNameBuffer[256] = {};
	strncpy(envMapNameBuffer, envMapName.c_str(), sizeof(envMapNameBuffer) - 1);
	ImGui::InputText("##envmapfilepath", envMapNameBuffer, sizeof(envMapNameBuffer), ImGuiInputTextFlags_ReadOnly);
	if (ImGui::IsItemHovered() && !EnvMapVulkanRenderer::GetEnvironmentMapFile().empty())
	{
		ImGui::SetTooltip("%s", EnvMapVulkanRenderer::GetEnvironmentMapFile().c_str());
	}

	// Drop an .hdr file from the Content Browser here to load it
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
			Log::GetLogger()->debug("END DRAG & DROP FILE '{0}'", itemPath);
			if (std::filesystem::path(itemPath).extension() == ".hdr")
			{
				RequestEnvironmentMap(itemPath);
			}
			else
			{
				Log::GetLogger()->warn("Only .hdr files can be used as environment maps ('{0}')", itemPath);
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::NextColumn();

	if (ImGui::Button("Load Environment Map"))
	{
		std::string filepath = FileDialogs::OpenFile({ { "HDR environment map", "hdr" } }, GetAssetFolderForDialog());
		if (!filepath.empty())
		{
			RequestEnvironmentMap(filepath); // loaded at the start of the next frame (see EditorPreFrame)
		}
	}

	ImGui::NextColumn();

	ImGui::AlignTextToFramePadding();

	// Skybox blur: 0 = sharp, up to the environment map's last mip level (a single average color)
	float maxSkyboxLod = EnvMapVulkanRenderer::GetEnvironmentMaxLod();
	ImGuiWrapper::Property("Skybox LOD", environment.SkyboxLod, 0.01f, 0.0f, maxSkyboxLod, PropertyFlag::DragProperty);

	ImGuiWrapper::Property("Exposure", environment.Exposure, 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);
	ImGuiWrapper::Property("Auto Exposure", environment.AutoExposure);
	ImGuiWrapper::Property("Extract Sun", environment.ExtractSun); // the map is loaded again, with or without its sun
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(EnvMapVulkanRenderer::GetExtractedSun().Found
			? "This map's sun is moved to the directional sun (intensity %.2f), so it casts shadows with all its light\nand isn't counted twice. Off: the map keeps its sun (the directional sun adds to it)."
			: "A real sun in a map (far brighter than the rest) is moved to the directional sun,\nso it casts shadows with all its light. This map has no sun.", EnvMapVulkanRenderer::GetExtractedSun().Intensity);
	}
	ImGuiWrapper::Property("Preserve Hue", environment.HuePreservation, 0.01f, 0.0f, 1.0f, PropertyFlag::DragProperty);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How bright colors are tonemapped:\n0 = per channel (a bright colored light turns white at its center, as on film)\n1 = hue-preserving (keeps the light's color all the way to the center)");
	}
	// No drag limits (min = max = 0): the value wraps around, so it can be dragged endlessly in both directions
	if (ImGuiWrapper::Property("Env Map Rotation", environment.Rotation, 1.0f, 0.0f, 0.0f, PropertyFlag::DragProperty))
	{
		environment.Rotation = std::fmod(environment.Rotation, 360.0f);
		if (environment.Rotation < 0.0f)
		{
			environment.Rotation += 360.0f;
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Turns the environment around the vertical axis (yaw, degrees):\nskybox, reflections and environment lighting together");
	}
	ImGui::Columns(1);
}

// The Environment panel: the environment map and display, the debug views, the selection overlays, ray tracing, the
// reflections, the ambient occlusion and the bloom
void OnImGuiRenderEnvironment()
{
	ImGui::Begin("Environment");
	{
		if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			{
				EnvironmentControls();
				ImGui::Columns(2);
				ImGuiWrapper::Property("Display Grid", s_ViewOptions.Grid);
				ImGuiWrapper::Property("Grid Scale", s_ViewOptions.GridScale, 0.1f, 1.0f, 256.0f, PropertyFlag::DragProperty);
				ImGuiWrapper::Property("Grid Line Width", s_ViewOptions.GridSize, 0.001f, 0.001f, 0.5f, PropertyFlag::DragProperty);
				ImGuiWrapper::Property("Show UI (Edit)", s_ViewOptions.UI);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("The game UI (the canvases) over the viewport while editing. Play always shows it");
				}
				ImGuiWrapper::Property("UI Outlines", s_ViewOptions.UIOutlines);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Debug view: the selected UI element's rectangle (orange), the rectangle it is laid out in\n"
						"(white), its anchors (blue squares) and its pivot (white ring)");
				}
				ImGuiWrapper::Property("Debug Overlay (F3)", s_DebugOverlay);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("The frame rate, the frame times, draw calls and the GPU's passes over the viewport, and while\n"
						"the game runs its physics, scripts and sounds: what F3 shows in the player");
				}

				ImGui::Columns(1);
			}
		}

		// What the viewport shows instead of the finished image: the G-buffer's contents, or one part of the lighting
		if (ImGui::CollapsingHeader("Debug View", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("DebugView");
			ImGui::Combo("View", &s_ViewOptions.DebugView, "Off\0Normals\0Roughness\0Linear Depth\0Motion\0Direct Light Only\0Indirect Light Only\0Diffuse Surface Shading\0Probe Irradiance Only\0Reflection Occlusion\0Ambient Occlusion\0AO Denoised\0AO Raw\0Ray Traced Normals\0Ray Traced Albedo\0Ray Traced Instances\0Reflections\0Reflections Raw\0Path Traced\0");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Normals, Roughness, Linear Depth, Motion: the data the G-buffer prepass writes for the opaque meshes\n"
					"  Normals: the world direction as a color (x red, y green, z blue; straight up is light green)\n"
					"  Roughness: black = smooth, white = rough\n"
					"  Linear Depth: the distance from the camera, black = near, white = Depth Range and beyond\n"
					"  Motion: movement on the screen since the previous frame. Gray = none, more red = to the right,\n"
					"  more green = down\n"
					"Direct Light Only: the sun, point and spot lights. Indirect Light Only: the environment's light.\n"
					"Diffuse Surface Shading: each surface as light that bounces sees it. One color per material (a map's\n"
					"  average), the mesh's own normals, no reflections: what the probes and ray hits will be lit with.\n"
					"  Opaque meshes only: the water and glass are drawn as usual.\n"
					"Probe Irradiance Only: the indirect diffuse light arriving at every surface, as a white matte surface\n"
					"  would show it: no textures, no direct light, no reflections. From the probes inside the probe volume,\n"
					"  from the environment elsewhere. For judging the global illumination by itself: leaks, blotches,\n"
					"  the color a wall gives its surroundings.\n"
					"Reflection Occlusion: how much of the environment's reflection the probe volume lets through at each\n"
					"  surface (its Occlude Reflections). White = all of it (open sky, or outside the volume),\n"
					"  black = none (a closed room).\n"
					"Ambient Occlusion: the screen-space occlusion by itself (white = open, black = closed in), as the\n"
					"  indirect light is multiplied by it. AO Denoised: the same before the average over the frames.\n"
					"  AO Raw: before the blur too, as the GTAO pass computes it for one frame (noisy).\n"
					"  Opaque meshes only. The settings: Ambient Occlusion, below.\n"
					"Ray Traced Normals, Albedo, Instances: one ray per pixel from the camera against the acceleration\n"
					"  structures, and what it hit, read the way every ray-traced effect reads it. Needs a GPU with ray tracing.\n"
					"  Normals: must look like the Normals view (a moved model or a playing animation must follow)\n"
					"  Albedo: the material's color or albedo map at the hit (the map at full size: it flickers far away)\n"
					"  Instances: a color per mesh in the top-level structure\n"
					"  Opaque meshes only (glass is in the structures, in a group these views leave out).\n"
					"Reflections: the ray-traced reflection image by itself: the light each surface reflects, as it arrives\n"
					"  (before the surface's own color and Fresnel). Black: no reflection was traced there (too rough, or the\n"
					"  sky), and the environment's reflection is used. Reflections Raw: the same as one frame's rays give it,\n"
					"  before the average over the frames and the blur. The settings: Reflections, below.\n"
					"Path Traced: the whole image by rays alone, as a reference. Every pixel's light is followed through all\n"
					"  its bounces: no probes, no ambient occlusion pass, no reflection image, no shadow maps. Noisy at first;\n"
					"  it gets cleaner with every frame while the camera and the scene stand still. What the normal view\n"
					"  should look like where its approximations hold. Needs Ray Tracing.");
			}
			if (s_ViewOptions.DebugView == DebugViewLinearDepth)
			{
				ImGui::DragFloat("Depth Range", &s_ViewOptions.DebugViewDepthRange, 0.5f, 1.0f, 1000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("The distance shown as white");
				}
			}
			if (s_ViewOptions.DebugView == DebugViewMotion)
			{
				ImGui::DragFloat("Motion Scale", &s_ViewOptions.DebugViewMotionScale, 0.1f, 0.5f, 200.0f, "%.1f px", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("The movement per frame shown at full color (lower: small movements show up more)");
				}
			}
			if (s_ViewOptions.DebugView == DebugViewPathTraced)
			{
				EnvMapVulkanPathTracerSettings& pathTracer = s_ViewOptions.PathTracer;
				ImGui::SliderInt("Max Bounces", &pathTracer.MaxBounces, 0, EnvMapVulkanPathTracerSettings::MaxMaxBounces, "%d", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("The times a path is scattered by a surface before it ends. 0: only the light straight from the\n"
						"lights and the sky (no bounced light, no reflections); 1: one bounce; more: the light that reaches\n"
						"deep into a room. Passing through glass doesn't count. Paths that carry little light end earlier.");
				}
				ImGui::SliderInt("Samples per Frame", &pathTracer.SamplesPerFrame, 1, EnvMapVulkanPathTracerSettings::MaxSamplesPerFrame, "%d", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Paths per pixel and frame. More: the image clears up in fewer frames, each of which takes longer\n"
						"(the editor gets less responsive).");
				}
				ImGui::DragInt("Max Samples", &pathTracer.MaxSamples, 16.0f, 0, 1 << 20, "%d", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("With this many paths per pixel the image is finished and nothing more is traced. 0: never.\n"
						"The noise halves with four times the samples.");
				}
				ImGui::DragFloat("Max Brightness", &pathTracer.MaxBrightness, 0.5f, 0.0f, 10000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("The most one path can add to a pixel after its first bounce. A rare path that finds a small, very\n"
						"bright spot leaves a white dot that takes thousands of samples to fade; cutting it off loses a little\n"
						"of the bounced light. 0: no limit (exact, with the dots).");
				}
				if (ImGui::Button("Restart"))
				{
					EnvMapVulkanRenderer::RestartPathTracer();
				}
				ImGui::SameLine();
				const uint32_t pathSamples = EnvMapVulkanRenderer::GetPathTracerSamples();
				const bool pathFinished = pathTracer.MaxSamples > 0 && pathSamples >= (uint32_t)pathTracer.MaxSamples;
				ImGui::TextDisabled(pathFinished ? "%u samples per pixel: finished" : "%u samples per pixel", pathSamples);
				ImGui::TextDisabled("No water, no bloom. Starts over when the camera or the scene changes.");
			}
			ImGui::PopID();
		}

		// Like "Display Outline / Wireframe / Bounding Boxes" in SceneHazelEnvMap
		if (ImGui::CollapsingHeader("Selection and Overlays", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			EnvMapVulkanOverlaySettings& overlay = s_ViewOptions.Overlays;
			ImGui::PushID("Overlays");

			ImGui::Checkbox("Selection Outline", &overlay.Outline);
			ImGui::BeginDisabled(!overlay.Outline);
			ImGui::DragFloat("Outline Width", &overlay.OutlineWidth, 0.05f, 1.0f, 10.0f, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
			ImGui::ColorEdit4("Outline Color", &overlay.OutlineColor.x, ImGuiColorEditFlags_NoInputs);
			ImGui::EndDisabled();

			ImGui::Separator();
			ImGui::Combo("Wireframe", &overlay.Wireframe, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Selected: the selected mesh, or the whole model when no mesh is selected (the water's grid\n"
					"when the water is selected)\nAll: every loaded model and the water");
			}
			ImGui::ColorEdit4("Wireframe Color", &overlay.WireframeColor.x, ImGuiColorEditFlags_NoInputs);

			ImGui::Separator();
			ImGui::Combo("Bounding Boxes", &overlay.BoundingBoxes, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Selected: the selected mesh's box, or all boxes of the model when no mesh is selected\nAll: the boxes of every loaded model");
			}
			ImGui::ColorEdit4("Selected Box", &overlay.SelectedBoundingBoxColor.x, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Box of the selected mesh\n(all boxes of the selected model when no mesh is selected)");
			}
			ImGui::SameLine();
			ImGui::ColorEdit4("Unselected Boxes", &overlay.BoundingBoxColor.x, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("All other boxes: other models, and the other meshes of the selected model.\n"
					"Only drawn with Bounding Boxes = All.");
			}

			ImGui::Separator();
			ImGui::Combo("Vertex Vectors", &overlay.Vectors, s_OverlayScopeNames, IM_ARRAYSIZE(s_OverlayScopeNames));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("A line per vertex along its normal, tangent and/or bitangent\n"
					"Selected: the selected mesh, or the whole model when no mesh is selected\nAll: every loaded model");
			}
			ImGui::BeginDisabled(overlay.Vectors == OverlayScopeOff);
			ImGui::Checkbox("Normals", &overlay.ShowNormals);
			ImGui::SameLine();
			ImGui::Checkbox("Tangents", &overlay.ShowTangents);
			ImGui::SameLine();
			ImGui::Checkbox("Bitangents", &overlay.ShowBitangents);
			ImGui::DragFloat("Vector Length", &overlay.VectorLength, 0.05f, 0.1f, 50.0f, "%.1f %% of model size", ImGuiSliderFlags_AlwaysClamp);
			static const char* s_VectorColorModes[] = { "By Vector", "By Direction" };
			ImGui::Combo("Vector Colors", &overlay.VectorColorMode, s_VectorColorModes, IM_ARRAYSIZE(s_VectorColorModes));
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("By Vector: tangent red, bitangent green, normal blue (tangent space x, y, z)\n"
					"By Direction: the world direction as a color (x red, y green, z blue)\n"
					"Lines are darker at the vertex and brighter at the tip");
			}
			ImGui::EndDisabled();

			ImGui::Separator();
			ImGui::DragFloat("Line Width", &overlay.LineWidth, 0.05f, 1.0f, 10.0f, "%.1f px", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Wireframe, bounding box and vertex vector lines");
			}

			ImGui::PopID();
		}

		// The master switch of everything ray-traced
		if (ImGui::CollapsingHeader("Ray Tracing", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("RayTracing");
			const bool rayTracingSupported = EnvMapVulkanRenderer::IsRayTracingSupported();
			ImGui::BeginDisabled(!rayTracingSupported);
			bool rayTracingOn = s_RenderSettings.RayTracing && rayTracingSupported;
			if (ImGui::Checkbox("Enabled", &rayTracingOn))
			{
				s_RenderSettings.RayTracing = rayTracingOn;
			}
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Everything that traces rays (hardware ray tracing: acceleration structures and ray queries).\n"
					"Off: no rays are traced and nothing is built or updated for them. Every effect uses its rasterized\n"
					"version, as on a GPU without ray tracing: for more frames per second when the rays cost too much.\n"
					"Used by: the probe volume's updates (rays instead of cube maps: much faster to follow a change),\n"
					"the reflections, the ambient occlusion (its Ray Traced setting) and the Debug View's Ray Traced views.\n"
					"Its cost: Ray Tracing Scene and the passes that trace rays, in Renderer Stats.");
			}
			if (!rayTracingSupported)
			{
				ImGui::TextDisabled("This GPU (or its driver) has no ray tracing");
			}
			else if (!s_RenderSettings.RayTracing)
			{
				ImGui::TextDisabled("Off: rasterized rendering only");
			}
			else if (IsRayTracedDebugView(s_ViewOptions.DebugView))
			{
				const EnvMapVulkanRayTracing::Stats& rayTracingStats = EnvMapVulkanRenderer::GetRayTracingStats();
				ImGui::TextDisabled("%u instances of %u meshes, %s triangles", rayTracingStats.Instances, rayTracingStats.Meshes,
					FormatCount(rayTracingStats.Triangles).c_str());
				ImGui::TextDisabled("%u skinned models refitted, %u textures, structures %.1f MB", rayTracingStats.SkinnedModels,
					rayTracingStats.Textures, (double)rayTracingStats.StructureBytes / (1024.0 * 1024.0));
			}
			else
			{
				std::string users;
				auto addUser = [&users](bool uses, const char* name)
				{
					if (uses)
					{
						users += users.empty() ? "" : ",\n";
						users += name;
					}
				};
				addUser(s_RenderSettings.Reflections.Enabled, "the reflections");
				addUser(s_RenderSettings.AmbientOcclusion.Enabled && s_RenderSettings.AmbientOcclusion.RayTraced, "the ambient occlusion");
				addUser(s_ProbeVolumeSettings.Exists, "the probe volume while it updates");
				addUser(s_ViewOptions.DebugView == DebugViewPathTraced, "the Path Traced view");
				ImGui::TextDisabled("Tracing rays: %s", users.empty() ? "nothing at the moment" : users.c_str());
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Reflections", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("Reflections");
			EnvMapVulkanReflectionSettings& reflections = s_RenderSettings.Reflections;
			const bool reflectionsAvailable = EnvMapVulkanRenderer::IsRayTracingActive();
			ImGui::BeginDisabled(!reflectionsAvailable);
			ImGui::Checkbox("Ray Traced", &reflections.Enabled);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("On: what a surface reflects is found by a ray against the scene, so it shows the room, the other\n"
					"objects and their light. Off (and without Ray Tracing): every surface reflects the environment map,\n"
					"which shows the sky only, darkened where the probe volume says the scene hides it.\n"
					"Opaque meshes. Glass and the water keep their own reflections; a reflection seen in a reflection\n"
					"is not followed (the reflected surface shows its matte light).\n"
					"Debug View > Reflections shows it by itself; its cost: Reflections in Renderer Stats.");
			}
			ImGui::BeginDisabled(!reflections.Enabled);
			ImGui::SliderFloat("Max Roughness", &reflections.MaxRoughness, EnvMapVulkanReflectionSettings::MinMaxRoughness,
				EnvMapVulkanReflectionSettings::MaxMaxRoughness, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Surfaces rougher than this get no ray and keep the environment's reflection (the traced one fades\n"
					"out over the last quarter below it). A rough surface's reflection is so blurred that it hardly shows\n"
					"what it reflects, and its rays scatter so widely that they are mostly noise. Higher: more surfaces\n"
					"reflect the scene, with more noise for the filters to remove.");
			}
			ImGui::Checkbox("Denoise", &reflections.Denoise);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Each pixel takes its reflection from the traced pixels of its surface around it, from a wider area\n"
					"the rougher the surface. Off: from the nearest ones only (a rough surface's reflection is grainy).");
			}
			ImGui::Checkbox("Temporal Filter", &reflections.Temporal);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Each frame's rays go in other directions, and the result is the average of the last frames (found\n"
					"again on moving surfaces through the G-buffer's motion). Off: one frame's rays, with a fixed pattern.\n"
					"The reflection follows the surface it is on, not what it shows: while the camera moves, a mirror's\n"
					"image trails a little.");
			}
			ImGui::Checkbox("Half Resolution", &reflections.HalfResolution);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("On: traced at half the viewport's size (a quarter of the rays) and scaled up along the edges: a mirror's\n"
					"image is half as sharp. Off: a ray for every pixel, at about four times the time.");
			}
			ImGui::EndDisabled();
			ImGui::EndDisabled();
			if (!reflectionsAvailable)
			{
				ImGui::TextDisabled("Needs Ray Tracing: the environment's reflection");
			}
			else if (EnvMapVulkanRenderer::AreReflectionsTraced())
			{
				ImGui::TextDisabled("Traced at %u x %u", EnvMapVulkanRenderer::GetReflectionsSize().x, EnvMapVulkanRenderer::GetReflectionsSize().y);
			}
			ImGui::PopID();
		}

		if (ImGui::CollapsingHeader("Ambient Occlusion", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID("AmbientOcclusion");
			EnvMapVulkanSSAOSettings& occlusion = s_RenderSettings.AmbientOcclusion;
			ImGui::Checkbox("Enabled", &occlusion.Enabled);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Screen-space ambient occlusion (GTAO): corners, creases and the places where things touch get less\n"
					"of the indirect light (the environment's, the probes'), because the geometry around them is in its way.\n"
					"The lights' own light isn't changed: that is what their shadows are for.\n"
					"Computed from what is on the screen: what is hidden or outside the view doesn't shade.\n"
					"Debug View > Ambient Occlusion shows it by itself; its cost: Renderer Stats.");
			}
			ImGui::BeginDisabled(!occlusion.Enabled);
			ImGui::SliderFloat("Radius", &occlusion.Radius, 0.05f, 5.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("How far from a point the geometry still shades it.\n"
					"With a probe volume keep it small (about 0.5 m): the probes already know the shade of everything\n"
					"larger, and a large radius would darken the same places twice.\n"
					"Without probes a larger radius (1 to 2 m) gives rooms and corners their depth.");
			}
			ImGui::SliderFloat("Power", &occlusion.Power, 0.5f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The strength: the occlusion to this power. 1 = as computed, higher = darker.");
			}
			ImGui::SliderFloat("Falloff", &occlusion.Falloff, 0.05f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The part of the radius over which geometry stops counting. 1: fading from the point on (soft);\n"
					"small: everything within the radius counts fully (stronger, with a harder end).");
			}
			// Which way the occlusion is found: by rays where they can be traced, otherwise from the depth buffer
			const bool occlusionTraced = occlusion.RayTraced && EnvMapVulkanRenderer::IsRayTracingActive();
			ImGui::BeginDisabled(!EnvMapVulkanRenderer::IsRayTracingActive());
			ImGui::Checkbox("Ray Traced", &occlusion.RayTraced);
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("On: a few short rays per pixel against the scene's real geometry. It also finds what the screen\n"
					"doesn't show: geometry hidden behind other geometry, or outside the view.\n"
					"Off: from the depth buffer (GTAO, with Slices and Steps): what is on the screen, and nothing else.\n"
					"Needs Ray Tracing (above) and a GPU that has it; without them it is always the depth buffer's way.");
			}
			ImGui::BeginDisabled(!occlusionTraced);
			ImGui::SliderInt("Rays", &occlusion.Rays, 1, EnvMapVulkanSSAOSettings::MaxRays, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Ray traced: rays per pixel and frame. More rays: less noise for the blur and the temporal filter to\n"
					"remove (AO Raw in Debug View shows it), at a cost that grows with the number.");
			}
			ImGui::BeginDisabled(occlusionTraced); // the depth buffer's way
			ImGui::SliderInt("Slices", &occlusion.Slices, 1, EnvMapVulkanSSAOSettings::MaxSlices, "%d", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Directions a pixel looks in per frame. More: less noise before the filters, more time.");
			}
			ImGui::SliderInt("Steps", &occlusion.Steps, 2, EnvMapVulkanSSAOSettings::MaxSteps, "%d", ImGuiSliderFlags_AlwaysClamp);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Depth lookups per direction and side. More: thin things within the radius are missed less often.");
			}
			ImGui::EndDisabled();
			ImGui::Checkbox("Denoise", &occlusion.Denoise);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("A 5 x 5 blur that stops at edges (it follows each pixel's surface, by its depth and normal).");
			}
			ImGui::SameLine();
			ImGui::Checkbox("Temporal Filter", &occlusion.Temporal);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Each frame looks in other directions, and the result is the average of the last frames\n"
					"(found again on moving surfaces through the G-buffer's motion). Off: one frame's result, with a fixed pattern.");
			}
			ImGui::Checkbox("Half Resolution", &occlusion.HalfResolution);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("On: computed at half the viewport's size (a quarter of the pixels) and scaled up along the edges.\n"
					"Thin dark lines (along a crease, beside an edge) then show the steps of the half-size pixels.\n"
					"Off: computed for every pixel. About four times the time: see Ambient Occlusion in Renderer Stats.");
			}
			ImGui::EndDisabled();
			ImGui::TextDisabled("%s at %u x %u", EnvMapVulkanRenderer::IsAmbientOcclusionRayTraced() ? "Ray traced" : "From the depth buffer", EnvMapVulkanRenderer::GetAmbientOcclusionSize().x, EnvMapVulkanRenderer::GetAmbientOcclusionSize().y);
			ImGui::PopID();
		}

		// Same settings as "Bloom Settings" in SceneHazelEnvMap's Scene Renderer panel
		if (ImGui::CollapsingHeader("Bloom Settings", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Columns(2);
			ImGui::AlignTextToFramePadding();
			ImGuiWrapper::Property("Bloom Enabled", s_RenderSettings.Bloom.Enabled);
			ImGuiWrapper::Property("Threshold", s_RenderSettings.Bloom.Threshold, 0.01f, 0.0f, 20.0f, PropertyFlag::DragProperty);
			ImGuiWrapper::Property("Knee", s_RenderSettings.Bloom.Knee, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);
			ImGuiWrapper::Property("Upsample Scale", s_RenderSettings.Bloom.UpsampleScale, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);
			ImGuiWrapper::Property("Intensity", s_RenderSettings.Bloom.Intensity, 0.05f, 0.0f, 20.0f, PropertyFlag::DragProperty);
			ImGuiWrapper::Property("Lens Dirt", s_RenderSettings.Bloom.DirtEnabled);
			ImGui::BeginDisabled(!s_RenderSettings.Bloom.DirtEnabled);
			ImGuiWrapper::Property("Dirt Intensity", s_RenderSettings.Bloom.DirtIntensity, 0.05f, 0.0f, 20.0f, PropertyFlag::DragProperty);
			ImGui::EndDisabled();
			ImGui::Columns(1);

			// Lens dirt texture: click the thumbnail (or drop an image on it) to choose another one
			ImTextureID dirtThumbnail = EnvMapVulkanRenderer::GetBloomDirtImage();
			bool clicked = dirtThumbnail ? ImGui::ImageButton("##bloomdirt", dirtThumbnail, ImVec2(64.0f, 64.0f), ImVec2(0, 1), ImVec2(1, 0))
				: ImGui::Button("Dirt", ImVec2(64.0f, 64.0f));
			if (clicked)
			{
				std::string filepath = FileDialogs::OpenFile({ { "Images", "png,jpg,jpeg,tga,bmp" } }, GetAssetFolderForDialog());
				if (!filepath.empty())
				{
					EnvMapVulkanRenderer::SetBloomDirtTexture(filepath);
				}
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					std::string itemPath = Util::to_str((const wchar_t*)payload->Data);
					if (IsImageFile(itemPath))
					{
						EnvMapVulkanRenderer::SetBloomDirtTexture(itemPath);
					}
				}
				ImGui::EndDragDropTarget();
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Lens dirt: %s\nClick to load another image, or drop one from the Content Browser", EnvMapVulkanRenderer::GetBloomDirtTextureFile().c_str());
			}
			ImGui::SameLine();
			ImGui::TextDisabled("Lens dirt texture");
		}
	}
	ImGui::End();
}
