#pragma once

#include "EnvMapVulkanComponents.h"
#include "EnvMapVulkanPathTracer.h"

#include <glm/glm.hpp>

#include <vector>


// Debug views: what the viewport shows instead of the finished image. The G-buffer views are drawn by the viewport
// composite (ViewportComposite.glsl, GBufferView), the lighting views by the PBR shaders (u_ShadowDebug.z), the ray-traced
// views by a compute pass into an image the composite shows (EnvMapVulkanRayTracing).
enum EnvMapVulkanDebugView
{
	DebugViewOff = 0, DebugViewNormals, DebugViewRoughness, DebugViewLinearDepth, DebugViewMotion, DebugViewDirectLight, DebugViewIndirectLight,
	DebugViewSurfaceShading, DebugViewProbeIrradiance, DebugViewReflectionOcclusion, DebugViewAO, DebugViewAODenoised, DebugViewAORaw,
	DebugViewRayTracedNormals, DebugViewRayTracedAlbedo, DebugViewRayTracedInstances,
	DebugViewReflections, DebugViewReflectionsRaw, DebugViewPathTraced,
	DebugViewCount
};

inline bool IsRayTracedDebugView(int view)
{
	return view >= DebugViewRayTracedNormals && view <= DebugViewRayTracedInstances;
}

// The views of the ray-traced reflection image (drawn by the PBR shaders, as the lighting views)
inline bool IsReflectionDebugView(int view)
{
	return view == DebugViewReflections || view == DebugViewReflectionsRaw;
}

// The views that trace rays: not available without ray tracing
inline bool DebugViewNeedsRayTracing(int view)
{
	return IsRayTracedDebugView(view) || IsReflectionDebugView(view) || view == DebugViewPathTraced;
}

// Which meshes an overlay is drawn for
enum EnvMapVulkanOverlayScope { OverlayScopeOff = 0, OverlayScopeSelected = 1, OverlayScopeAll = 2 };

// Overlays drawn into the scene image for the selection: an outline, wireframes, bounding boxes, vertex vectors
struct EnvMapVulkanOverlaySettings
{
	bool Outline = true;
	float OutlineWidth = 2.0f; // pixels (1..10)
	glm::vec4 OutlineColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
	int Wireframe = OverlayScopeOff;
	glm::vec4 WireframeColor = glm::vec4(0.1f, 0.9f, 0.3f, 1.0f);
	int BoundingBoxes = OverlayScopeOff;
	glm::vec4 BoundingBoxColor = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
	glm::vec4 SelectedBoundingBoxColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // the selected mesh's box (all boxes of the selected model when no mesh is selected)
	float LineWidth = 1.0f; // wireframe, bounding boxes, and the normal/tangent/bitangent lines (pixels, 1..10)

	// Vertex vectors: a line per vertex along its normal, tangent and/or bitangent
	int Vectors = OverlayScopeOff;
	bool ShowNormals = true;
	bool ShowTangents = false;
	bool ShowBitangents = false;
	float VectorLength = 2.0f; // percent of the model's size (its bounding box diagonal), so it suits models of any scale
	int VectorColorMode = 0;   // 0: by vector (tangent red, bitangent green, normal blue), 1: by direction (xyz as rgb)
};

/**
 * How the renderer shows the scene apart from the finished image: debug views, the grid, the selection and its overlays.
 * The editor's view of the scene, changed in its panels; not saved with the scene. The defaults show nothing extra (a
 * game's view); Editor() gives what the editor starts with.
 */
struct EnvMapVulkanViewOptions
{
	int DebugView = DebugViewOff;
	float DebugViewDepthRange = 50.0f;  // Linear Depth: the distance shown as white (world units)
	float DebugViewMotionScale = 20.0f; // Motion: the motion shown at full color (pixels per frame)
	EnvMapVulkanPathTracerSettings PathTracer; // the Path Traced view

	bool ShowCascades = false; // the scene tinted by sun shadow cascade: red, green, blue, yellow (nearest to farthest)
	bool ShadowsOnly = false;  // the selected light's shadow alone (see ShadowDebugValue in the PBR shaders)

	bool Grid = false;
	float GridScale = 16.025f; // cells across the grid (it spans 32 x 32 units)
	float GridSize = 0.025f;   // line width, as a fraction of a cell

	EnvMapVulkanOverlaySettings Overlays = NoOverlays();

	// The editor's selection: drawn with the outline and the Selected overlays. The primary entity (one of them) is the
	// light the Shadows Only view shows, when it is a light.
	std::vector<EnvMapVulkanEntityID> Selection;
	EnvMapVulkanEntityID PrimarySelection = NoEntity;

	static EnvMapVulkanViewOptions Editor()
	{
		EnvMapVulkanViewOptions options;
		options.Grid = true;
		options.Overlays = EnvMapVulkanOverlaySettings();
		return options;
	}

private:
	static EnvMapVulkanOverlaySettings NoOverlays()
	{
		EnvMapVulkanOverlaySettings overlays;
		overlays.Outline = false;
		return overlays;
	}
};
