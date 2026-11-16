#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <vector>


/**
 * Debug drawings in the world (SceneEnvMapVulkan): colored triangles, lines and dots that the editor's debug views hand
 * to the renderer, drawn into the editor overlay (EnvMapVulkanRenderer's RecordEditorOverlayPasses) and hidden by the
 * scene: each pixel is tested against the scene's depth (the G-buffer's), so what is behind a wall or under a floor is
 * hidden, or shown faintly (HiddenAlpha). The Navigation debug view draws with it (src/Editor/EditorNavigation.cpp).
 *
 * Two lists:
 *  - the frame's list (Frame): filled again for each frame, dropped after it is drawn: what moves (paths, agents)
 *  - one persistent list (SetPersistent): kept on the GPU while its key stays the same, so a large mesh is uploaded once
 *    (the navmesh: its key is its NavMesh's serial). It has to be set again for every frame it is to be drawn: a frame
 *    without it frees it, so a scene's drawings go with the scene.
 *
 * Lines and dots have a width in pixels: the vertex stage makes them of two triangles facing the screen, with soft edges.
 * Colors are 32 bits, 0xAABBGGRR (the byte order of ImGui's IM_COL32). A color with alpha 0 is a palette entry: its low
 * six bits pick one of the frame's 64 palette colors (FrameSettings::Palette), for colors that change while the
 * persistent list stays (a Nav Area's polygons: green while the door is open, red when it's closed).
 */
struct EnvMapVulkanDebugDrawList
{
	// std430 vec4s, as DebugDraw.glsl reads them: positions in xyz, the colors' bits in w
	struct Triangle { glm::vec4 A, B, C; };          // A.w: the color
	struct Line { glm::vec4 A, B; };                 // A.w: the color, B.w: the width (pixels)
	struct Dot { glm::vec4 Center; glm::vec4 Style; }; // Center.w: the color; Style: x the radius (pixels), y the outline's color

	std::vector<Triangle> Triangles;
	std::vector<Line> Lines;
	std::vector<Dot> Dots;

	void AddTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, uint32_t color);
	void AddLine(const glm::vec3& a, const glm::vec3& b, uint32_t color, float width = 1.0f);
	// A disc facing the screen, with an outline 1.5 pixels wide (alpha 0: none)
	void AddDot(const glm::vec3& center, uint32_t color, float radius, uint32_t outline = 0xC8000000u);

	void Clear();
	bool IsEmpty() const { return Triangles.empty() && Lines.empty() && Dots.empty(); }
};

namespace EnvMapVulkanDebugDraw
{
	// How the next frame's drawings are drawn
	struct FrameSettings
	{
		uint32_t Palette[64] = {};
		float HiddenAlpha = 0.15f;    // what the scene hides is drawn with this much of its alpha (0: not at all)
		float DrawDistance = 0.0f;    // meters from the camera: farther, nothing is drawn (0: no limit); it fades over the last tenth
		float DepthTolerance = 0.05f; // meters (and 0.2% of the distance) a drawing may be behind the scene's surface and still count as on it
	};

	// The frame's list: filled for the next frame (cleared once that frame is drawn)
	EnvMapVulkanDebugDrawList& Frame();
	FrameSettings& Settings();

	// The persistent list for the next frame: kept while key is the one before; another key (or the first) has fill
	// called to make it. key 0: none
	void SetPersistent(uint64_t key, const std::function<void(EnvMapVulkanDebugDrawList&)>& fill);
}
