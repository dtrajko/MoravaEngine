// The Renderer Stats panel


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

// Colors of the top-level passes (the timeline and the table's markers)
static ImU32 GetPassColor(int index)
{
	static const ImU32 colors[] = {
		IM_COL32(86, 156, 214, 255), IM_COL32(220, 160, 70, 255), IM_COL32(120, 190, 110, 255), IM_COL32(200, 100, 160, 255),
		IM_COL32(90, 200, 200, 255), IM_COL32(230, 110, 90, 255), IM_COL32(160, 140, 230, 255), IM_COL32(200, 200, 90, 255),
		IM_COL32(150, 150, 160, 255), IM_COL32(100, 210, 160, 255), IM_COL32(240, 140, 180, 255), IM_COL32(130, 170, 90, 255),
	};
	return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}

std::string FormatCount(uint64_t value)
{
	char text[32];
	if (value >= 10000000ull)
	{
		snprintf(text, sizeof(text), "%.1fM", value / 1.0e6);
	}
	else if (value >= 10000ull)
	{
		snprintf(text, sizeof(text), "%.1fK", value / 1.0e3);
	}
	else
	{
		snprintf(text, sizeof(text), "%llu", (unsigned long long)value);
	}
	return text;
}

// The CPU and GPU frame times of the last frames as two lines, with the 60 and 30 FPS marks
static void DrawFrameTimeGraph(float height)
{
	const auto& cpu = EnvMapVulkanProfiler::GetCpuHistory();
	const auto& gpu = EnvMapVulkanProfiler::GetGpuHistory();
	const uint32_t offset = EnvMapVulkanProfiler::GetHistoryOffset();
	const uint32_t count = EnvMapVulkanProfiler::HistorySize;

	float maxMs = 1000.0f / 60.0f;
	for (uint32_t i = 0; i < count; i++)
	{
		maxMs = std::max(maxMs, std::max(cpu[i], gpu[i]));
	}
	maxMs = std::min(maxMs * 1.1f, 100.0f); // a stall (loading a model) doesn't flatten the rest

	const ImVec2 size(ImGui::GetContentRegionAvail().x, height);
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + size.x, min.y + size.y);
	ImGui::InvisibleButton("##FrameTimeGraph", size);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, IM_COL32(25, 25, 30, 255), 3.0f);
	drawList->PushClipRect(min, max, true);

	auto yOf = [&](float ms) { return max.y - (std::min(ms, maxMs) / maxMs) * (size.y - 2.0f) - 1.0f; };
	for (float fps : { 60.0f, 30.0f })
	{
		const float ms = 1000.0f / fps;
		if (ms < maxMs)
		{
			const float y = yOf(ms);
			drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), IM_COL32(255, 255, 255, 50));
			char label[32];
			snprintf(label, sizeof(label), "%.0f FPS", fps);
			drawList->AddText(ImVec2(min.x + 4.0f, y - ImGui::GetTextLineHeight()), IM_COL32(255, 255, 255, 90), label);
		}
	}
	auto drawLine = [&](const std::array<float, EnvMapVulkanProfiler::HistorySize>& values, ImU32 color) {
		std::vector<ImVec2> points;
		points.reserve(count);
		for (uint32_t i = 0; i < count; i++)
		{
			const float x = min.x + size.x * (float)i / (float)(count - 1);
			points.push_back(ImVec2(x, yOf(values[(offset + i) % count])));
		}
		drawList->AddPolyline(points.data(), (int)points.size(), color, ImDrawFlags_None, 1.5f);
	};
	const ImU32 cpuColor = IM_COL32(230, 180, 80, 255);
	const ImU32 gpuColor = IM_COL32(90, 170, 240, 255);
	drawLine(cpu, cpuColor);
	drawLine(gpu, gpuColor);
	drawList->PopClipRect();

	if (ImGui::IsItemHovered())
	{
		// The frame under the cursor
		const float t = (ImGui::GetIO().MousePos.x - min.x) / size.x;
		const uint32_t i = (uint32_t)glm::clamp(t * (count - 1) + 0.5f, 0.0f, (float)(count - 1));
		const float cpuMs = cpu[(offset + i) % count];
		const float gpuMs = gpu[(offset + i) % count];
		ImGui::SetTooltip("%u frames ago\nFrame (CPU): %.2f ms (%.0f FPS)\nGPU: %.2f ms", count - 1 - i, cpuMs, cpuMs > 0.0f ? 1000.0f / cpuMs : 0.0f, gpuMs);
	}
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(cpuColor), "Frame (CPU)");
	ImGui::SameLine();
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(gpuColor), "GPU");
	ImGui::SameLine();
	ImGui::TextDisabled("last %u frames, up to %.1f ms", count, maxMs);
}

// The GPU frame as a bar: each top-level pass as wide as its share of the GPU time
static void DrawGpuTimeline(const EnvMapVulkanProfiler::FrameResults& results)
{
	double total = 0.0;
	for (const auto& scope : results.Scopes)
	{
		if (scope.Depth == 0)
		{
			total += scope.SmoothedGpuMs;
		}
	}
	const ImVec2 size(ImGui::GetContentRegionAvail().x, 18.0f);
	const ImVec2 min = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##GpuTimeline", size);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), IM_COL32(25, 25, 30, 255), 3.0f);
	if (total <= 0.0)
	{
		return;
	}
	const float mouseX = ImGui::GetIO().MousePos.x;
	const bool hovered = ImGui::IsItemHovered();
	float x = min.x;
	int top = 0;
	for (const auto& scope : results.Scopes)
	{
		if (scope.Depth != 0)
		{
			continue;
		}
		const float width = (float)(scope.SmoothedGpuMs / total) * size.x;
		const ImVec2 a(x, min.y), b(x + width, min.y + size.y);
		drawList->AddRectFilled(a, b, GetPassColor(top));
		if (width > 40.0f)
		{
			drawList->PushClipRect(a, b, true);
			drawList->AddText(ImVec2(a.x + 3.0f, a.y + 2.0f), IM_COL32(20, 20, 20, 255), scope.Name.c_str());
			drawList->PopClipRect();
		}
		if (hovered && mouseX >= a.x && mouseX < b.x)
		{
			ImGui::SetTooltip("%s: %.3f ms (%.0f%% of the GPU frame)", scope.Name.c_str(), scope.SmoothedGpuMs, 100.0 * scope.SmoothedGpuMs / total);
		}
		x += width;
		top++;
	}
}

// The tooltip of a pass: what it does, and the GPU's statistics for a top-level one
static void PassTooltip(const EnvMapVulkanProfiler::ScopeResult& scope)
{
	ImGui::BeginTooltip();
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
	ImGui::Text("%s", scope.Name.c_str());
	if (scope.Description)
	{
		ImGui::Separator();
		ImGui::TextUnformatted(scope.Description);
	}
	if (scope.HasStatistics)
	{
		using P = EnvMapVulkanProfiler;
		ImGui::Separator();
		ImGui::TextDisabled("Pipeline statistics (counted by the GPU)");
		ImGui::Text("Vertices read: %s", FormatCount(scope.Statistics[P::InputVertices]).c_str());
		ImGui::Text("Vertex shader runs: %s", FormatCount(scope.Statistics[P::VertexShaderInvocations]).c_str());
		ImGui::Text("Primitives assembled: %s", FormatCount(scope.Statistics[P::InputPrimitives]).c_str());
		ImGui::Text("Primitives rasterized: %s (the rest were culled or off screen)", FormatCount(scope.Statistics[P::ClippingPrimitives]).c_str());
		ImGui::Text("Fragment shader runs: %s", FormatCount(scope.Statistics[P::FragmentShaderInvocations]).c_str());
		const uint64_t pixels = (uint64_t)scope.TargetWidth * scope.TargetHeight;
		if (pixels > 0 && scope.RenderPasses == 1)
		{
			ImGui::Text("Shading per pixel: %.2fx (%u x %u target)", (double)scope.Statistics[P::FragmentShaderInvocations] / pixels,
				scope.TargetWidth, scope.TargetHeight);
			ImGui::TextDisabled("Above 1: overdraw, pixels shaded more than once");
		}
	}
	ImGui::PopTextWrapPos();
	ImGui::EndTooltip();
}

void OnImGuiRenderRendererStats()
{
	if (!s_ShowRendererStats)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(460.0f, 640.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Renderer Stats", &s_ShowRendererStats))
	{
		ImGui::End();
		return;
	}
	const EnvMapVulkanProfiler::FrameResults& results = EnvMapVulkanProfiler::GetResults();
	const double cpuMs = EnvMapVulkanProfiler::GetSmoothedCpuFrameMs();
	const double gpuMs = results.SmoothedGpuMs;

	// The frame rate, and what limits it
	ImGui::SetWindowFontScale(1.6f);
	ImGui::Text("%.0f FPS", EnvMapVulkanProfiler::GetFps());
	ImGui::SetWindowFontScale(1.0f);
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::Text("Frame: %.2f ms", cpuMs);
	if (results.Valid)
	{
		ImGui::Text("GPU:   %.2f ms", gpuMs);
	}
	else
	{
		ImGui::TextDisabled("GPU: no timestamps on this device");
	}
	ImGui::EndGroup();
	if (results.Valid && cpuMs > 0.0)
	{
		const bool gpuBound = gpuMs > cpuMs * 0.75;
		ImGui::TextColored(gpuBound ? ImVec4(0.55f, 0.75f, 1.0f, 1.0f) : ImVec4(1.0f, 0.8f, 0.45f, 1.0f), gpuBound ? "GPU-bound" : "CPU-bound");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(gpuBound
				? "The GPU's work takes most of the frame: fewer or cheaper pixels and passes (resolution, shadows, reflections) "
				  "raise the frame rate."
				: "The GPU finishes early and waits: the frame time is the CPU's (the editor UI, scene updates, recording "
				  "commands, waiting for the GPU at the end of each frame). Cheaper GPU work won't raise the frame rate much.");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(GPU busy %.0f%% of the frame)", 100.0 * std::min(gpuMs / cpuMs, 1.0));
	}
	ImGui::Checkbox("FPS in the viewport", &s_ShowViewportFps);

	ImGui::Spacing();
	DrawFrameTimeGraph(80.0f);

	// The passes
	ImGui::Spacing();
	ImGui::SeparatorText("GPU passes (last finished frame)");
	if (!results.Valid)
	{
		ImGui::TextDisabled("The GPU's timestamps aren't available: only the counts are shown");
	}
	DrawGpuTimeline(results);
	ImGui::TextDisabled("%u draw calls, %s triangles, %u render passes", results.Draws, FormatCount(results.Triangles).c_str(), results.RenderPasses);

	// Top-level passes that ran before but not in this frame (turned off: no water, no point light shadows...)
	static std::vector<std::pair<std::string, const char*>> s_KnownPasses;
	for (const auto& scope : results.Scopes)
	{
		if (scope.Depth == 0 && std::find_if(s_KnownPasses.begin(), s_KnownPasses.end(), [&](const auto& p) { return p.first == scope.Name; }) == s_KnownPasses.end())
		{
			s_KnownPasses.push_back({ scope.Name, scope.Description });
		}
	}

	const ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
	if (ImGui::BeginTable("##Passes", 6, tableFlags))
	{
		ImGui::TableSetupColumn("Pass", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("GPU ms");
		ImGui::TableSetupColumn("%");
		ImGui::TableSetupColumn("Draws");
		ImGui::TableSetupColumn("Triangles");
		ImGui::TableSetupColumn("Target");
		ImGui::TableHeadersRow();

		const double total = gpuMs > 0.0 ? gpuMs : 1.0;
		int top = -1;
		// The scopes are in recording order, a parent before its children: a tree node per scope; a closed node skips its
		// children (the scopes after it with a greater depth), an open one is popped when its subtree ends
		const auto& scopes = results.Scopes;
		std::vector<int> openDepths; // the open tree nodes that have children
		for (size_t i = 0; i < scopes.size();)
		{
			const auto& scope = scopes[i];
			top += scope.Depth == 0 ? 1 : 0;
			const bool hasChildren = i + 1 < scopes.size() && scopes[i + 1].Depth > scope.Depth;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::PushID((int)i);
			if (scope.Depth == 0)
			{
				// The pass's color in the timeline
				const ImVec2 p = ImGui::GetCursorScreenPos();
				const float h = ImGui::GetTextLineHeight();
				ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y + 2.0f), ImVec2(p.x + 4.0f, p.y + h - 2.0f), GetPassColor(top));
				ImGui::SetCursorScreenPos(ImVec2(p.x + 8.0f, p.y));
			}
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
			flags |= hasChildren ? (scope.Depth == 0 ? ImGuiTreeNodeFlags_DefaultOpen : 0) : (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			const bool open = ImGui::TreeNodeEx(scope.Name.c_str(), flags);
			if (hasChildren && open)
			{
				openDepths.push_back(scope.Depth);
			}
			if (ImGui::IsItemHovered())
			{
				PassTooltip(scope);
			}
			ImGui::TableNextColumn();
			ImGui::Text("%.3f", scope.SmoothedGpuMs);
			ImGui::TableNextColumn();
			ImGui::Text("%.0f", 100.0 * scope.SmoothedGpuMs / total);
			ImGui::TableNextColumn();
			ImGui::Text("%u", scope.Draws);
			ImGui::TableNextColumn();
			ImGui::Text("%s", FormatCount(scope.Triangles).c_str());
			ImGui::TableNextColumn();
			if (scope.TargetWidth > 0)
			{
				ImGui::Text(scope.RenderPasses > 1 ? "%ux%u (x%u)" : "%ux%u", scope.TargetWidth, scope.TargetHeight, scope.RenderPasses);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%u render pass(es); the last one drew into a %u x %u image", scope.RenderPasses, scope.TargetWidth, scope.TargetHeight);
				}
			}
			else
			{
				ImGui::TextDisabled("-");
			}
			ImGui::PopID();

			size_t next = i + 1;
			if (hasChildren && !open)
			{
				while (next < scopes.size() && scopes[next].Depth > scope.Depth)
				{
					next++;
				}
			}
			const int nextDepth = next < scopes.size() ? scopes[next].Depth : -1;
			while (!openDepths.empty() && openDepths.back() >= nextDepth)
			{
				ImGui::TreePop();
				openDepths.pop_back();
			}
			i = next;
		}

		// Passes that didn't run in this frame
		for (const auto& [name, description] : s_KnownPasses)
		{
			bool ran = false;
			for (const auto& scope : scopes)
			{
				ran |= scope.Depth == 0 && scope.Name == name;
			}
			if (ran)
			{
				continue;
			}
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("    %s (off)", name.c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Not running in this frame%s%s", description ? ":\n" : "", description ? description : "");
			}
		}
		ImGui::EndTable();
	}
	ImGui::TextDisabled("Hover a pass for what it does and the GPU's counts");

	// The scene
	ImGui::Spacing();
	ImGui::SeparatorText("Scene");
	uint32_t modelCount = 0, meshCount = 0;
	uint64_t triangleCount = 0;
	for (EnvMapVulkanEntityID model : GetModels(s_Scene))
	{
		if (H2M::RefH2M<H2M::ModelH2M> modelRef = GetModel(s_Scene, model))
		{
			modelCount++;
			for (const auto& mesh : modelRef->GetMeshes())
			{
				meshCount++;
				triangleCount += mesh->IndexCount / 3;
			}
		}
	}
	ImGui::Text("%u models, %u meshes, %s triangles", modelCount, meshCount, FormatCount(triangleCount).c_str());

	// Frustum culling of the main view (this frame's recording, a frame ahead of the passes above)
	const EnvMapVulkanRenderer::CullingStats& culling = EnvMapVulkanRenderer::GetCullingStats();
	ImGui::Text("Main view: %u of %u meshes drawn, %u culled", culling.Meshes - culling.Culled, culling.Meshes, culling.Culled);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Frustum culling: before the G-Buffer Prepass and the Scene pass, each mesh's bounding box (turned with\n"
			"the mesh) is tested against the camera's view; a mesh entirely outside it isn't drawn in them.\n"
			"Skinned meshes are always drawn (%u of them): their bones move them out of their boxes.\n"
			"The shadow maps cull against their own views; the water's reflection and ray tracing see every mesh.", culling.Skinned);
	}
	ImGui::Checkbox("Frustum Culling", &s_ViewOptions.FrustumCulling);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Off: every mesh is drawn, to compare the frame times and to check that culling changes nothing on screen");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Freeze", &s_ViewOptions.FreezeCulling);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Keeps culling against the view of the moment it was turned on: move the camera out of it to see\n"
			"what is culled (with Show Culled: the frozen view as a yellow frustum)");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Show Culled", &s_ViewOptions.ShowCulled);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The bounding boxes of the culled meshes in red, and the frozen view's frustum in yellow");
	}
	uint32_t shadowedPointLights = 0, shadowedSpotLights = 0;
	EnvMapVulkanRenderer::GetShadowedLightCounts(shadowedPointLights, shadowedSpotLights);
	ImGui::Text("%zu point lights (%u with shadows), %zu spot lights (%u with shadows)", s_Lights.PointLights.size(), shadowedPointLights,
		s_Lights.SpotLights.size(), shadowedSpotLights);
	ImGui::Text("Viewport: %u x %u", EnvMapVulkanRenderer::GetViewportWidth(), EnvMapVulkanRenderer::GetViewportHeight());

	// The GPU
	ImGui::Spacing();
	ImGui::SeparatorText("GPU");
	const EnvMapVulkanRenderer::GPUInfo gpu = EnvMapVulkanRenderer::GetGPUInfo();
	ImGui::Text("%s", gpu.Name.c_str());
	ImGui::Text("%s", gpu.API.c_str());
	if (!gpu.Driver.empty())
	{
		ImGui::SameLine();
		ImGui::Text(", driver %s", gpu.Driver.c_str());
	}
	ImGui::TextDisabled("Timestamps: %s, pipeline statistics: %s", EnvMapVulkanProfiler::IsSupported() ? "yes" : "no",
		EnvMapVulkanProfiler::AreStatisticsSupported() ? "yes" : "no");

	ImGui::End();
}
