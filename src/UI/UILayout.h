#pragma once

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class EnvMapVulkanScene;
class UIFont;


// One corner of a UI quad, in the view's pixels (top-left origin, y down). Mode 0: the texture's color times Color (no
// texture: a white texel); 1: text, the texture a font atlas whose median distance gives the coverage, DistancePixels
// the atlas's distance range in screen pixels at this glyph's size; 2: as 0, the texture sRGB (encoded again: its texels
// as they are); 3: as 0, the texture HDR light (exposed and tonemapped as the scene) (Resources/Shaders/UI.glsl)
struct UIVertex
{
	glm::vec2 Position;
	glm::vec2 TexCoord;
	glm::vec4 Color;
	float Mode;
	float DistancePixels;
};

// Indices that draw with one texture (null: the white texel), in the order they are drawn
struct UIDrawBatch
{
	H2M::RefH2M<H2M::Texture2D_H2M> Texture;
	uint32_t FirstIndex = 0;
	uint32_t IndexCount = 0;
};

// Where a rotated element's points are on the screen: view = Origin + Basis * local (no rotation: the same point)
struct UITransform
{
	glm::vec2 Origin = glm::vec2(0.0f);
	glm::mat2 Basis = glm::mat2(1.0f); // a rotation: its inverse is its transpose

	bool IsIdentity() const { return Origin == glm::vec2(0.0f) && Basis == glm::mat2(1.0f); }
	glm::vec2 ToView(const glm::vec2& local) const { return Origin + Basis * local; }
	glm::vec2 ToLocal(const glm::vec2& view) const { return glm::transpose(Basis) * (view - Origin); }
	// This one turned by degrees (clockwise on the screen) around a point of its local space
	UITransform Rotated(float degrees, const glm::vec2& center) const;
};

// Where an element ended up (view pixels), for the debug view, the editor and the mouse (UI/SceneUI). With a Rotation
// (its own or a parent's) the rectangles are in their local space: Min, Max and Pivot in Transform's, the parent's
// rectangle and the anchors in ParentTransform's; without one both are the view itself
struct UIElementRect
{
	EnvMapVulkanEntityID Entity = NoEntity;
	EnvMapVulkanEntityID Canvas = NoEntity;
	glm::vec2 Min = glm::vec2(0.0f);
	glm::vec2 Max = glm::vec2(0.0f);
	glm::vec2 ParentMin = glm::vec2(0.0f);    // the rectangle it is laid out in
	glm::vec2 ParentMax = glm::vec2(0.0f);
	glm::vec2 AnchorMin = glm::vec2(0.0f);    // its anchors in that rectangle
	glm::vec2 AnchorMax = glm::vec2(0.0f);
	glm::vec2 Pivot = glm::vec2(0.0f);        // the point Position placed
	float Scale = 1.0f;                       // the canvas's: view pixels per reference pixel
	UITransform Transform;                    // its own rotation and its parents'
	UITransform ParentTransform;              // its parents' only

	// The point (view pixels) is in its rectangle, rotated as it is drawn
	bool Contains(const glm::vec2& point) const
	{
		const glm::vec2 local = Transform.ToLocal(point);
		return local.x >= Min.x && local.y >= Min.y && local.x < Max.x && local.y < Max.y;
	}
	glm::vec2 GetCenter() const { return Transform.ToView((Min + Max) * 0.5f); }
};

// What the UI draws in a frame: quads (four vertices, six indices each) in batches, and the elements' rectangles
struct UIDrawList
{
	std::vector<UIVertex> Vertices;
	std::vector<uint32_t> Indices;
	std::vector<UIDrawBatch> Batches;
	std::vector<UIElementRect> Elements;
	uint32_t TextQuads = 0;
	uint32_t ImageQuads = 0;

	void Clear();
	bool IsEmpty() const { return Indices.empty(); }
	// A quad (corners in view pixels, texture coordinates from min to max), joined to the last batch when it has the texture
	void AddQuad(const glm::vec2& min, const glm::vec2& max, const glm::vec2& uvMin, const glm::vec2& uvMax, const glm::vec4& color,
		const H2M::RefH2M<H2M::Texture2D_H2M>& texture, float mode = 0.0f, float distancePixels = 0.0f);
	// A rectangle's outline, width pixels wide, inside it
	void AddOutline(const glm::vec2& min, const glm::vec2& max, float width, const glm::vec4& color);
	// Moves the vertices from firstVertex on (local points) to where the transform puts them on the screen
	void TransformVertices(size_t firstVertex, const UITransform& transform);
};

// Where a slider's parts are in view pixels (UISliderComponent): its handle's center goes from TravelMin to TravelMax (x)
// at CenterY, so the handle stays inside the element; HalfHandle is half the handle's size
struct UISliderGeometry
{
	float TravelMin = 0.0f, TravelMax = 0.0f;
	float CenterY = 0.0f;
	glm::vec2 HalfHandle = glm::vec2(0.0f);
};

// Where an open drop-down's list is in view pixels (UIDropdownComponent): Count rows of RowHeight, the first at Min.y
struct UIDropdownListGeometry
{
	glm::vec2 Min = glm::vec2(0.0f), Max = glm::vec2(0.0f);
	float RowHeight = 0.0f;
	int Count = 0;

	// The row at the point (-1: none)
	int RowAt(const glm::vec2& point) const;
};

/**
 * Lays out the game UI (docs/engine/Game_Engine_Runtime_Guide.html, 7.1 and 7.2): every canvas of the scene for a view of
 * viewSize pixels, its elements' rectangles from their anchors (UIRectComponent), then their images and text as quads.
 * No graphics API here: the renderer draws the list (EnvMapVulkanUI).
 *
 * Canvases are drawn by SortOrder, then in the scene's order; in a canvas, an element before its children and its
 * children in order (later ones on top), an element's image before its text. A slider: its image (the track), its fill
 * and its handle, then its text. A drop-down: its image, its chosen option and its arrow; its open list is drawn last,
 * over every canvas. An entity under a canvas without a
 * UIRectComponent is a group: its children are laid out in its parent's rectangle. A canvas under another canvas is a
 * canvas of its own (the whole view).
 *
 * A UI Rect's Rotation (degrees, clockwise on the screen) turns the element and its children around its pivot: a dial's
 * needle. Its quads are turned as they are made; the mouse finds it where it is drawn (UIElementRect::Contains).
 */
namespace UILayout
{
	// elementsOnly: only the elements' rectangles, no quads (the mouse's and the navigation's, UI/SceneUI)
	void Build(const EnvMapVulkanScene& scene, const glm::vec2& viewSize, UIDrawList& out, bool elementsOnly = false);

	// The debug view, added to the list: the selected element's rectangle, the rectangle it is laid out in, its anchors and
	// its pivot
	void AddSelectionOverlay(UIDrawList& list, EnvMapVulkanEntityID selected);
	// The debug view in a running game: the element under the mouse (green) and the selected button (yellow)
	void AddInteractionOverlay(UIDrawList& list, EnvMapVulkanEntityID hovered, EnvMapVulkanEntityID selected);

	// Text in a rectangle of view pixels, as a UI Text draws it (its Size times scale pixels per em): for text that isn't a
	// UI entity (the debug overlay, 7.4)
	void AddText(UIDrawList& list, const UITextComponent& text, const glm::vec2& min, const glm::vec2& max, float scale = 1.0f);
	// The width of a line of text in ems, kerning included
	float MeasureText(const UIFont& font, const std::string& line);

	// A slider's parts in its element's rectangle (view pixels; scale: its canvas's view pixels per reference pixel)
	UISliderGeometry GetSliderGeometry(const UISliderComponent& slider, const glm::vec2& min, const glm::vec2& max, float scale);
	// A drop-down's list for its element's rectangle (view pixels) in a view of viewSize: below the element, else above it,
	// else where it fits best
	UIDropdownListGeometry GetDropdownList(const UIDropdownComponent& dropdown, const glm::vec2& min, const glm::vec2& max, const glm::vec2& viewSize);

	// View pixels per reference pixel for this canvas in a view of that size
	float GetCanvasScale(const UICanvasComponent& canvas, const glm::vec2& viewSize);

	// The canvas the entity is drawn under (NoEntity: none, the entity isn't drawn); the entity itself when it is one
	EnvMapVulkanEntityID FindCanvas(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity);

	// Forgets the textures of UI images (and the files that couldn't be read, so they are tried again)
	void ClearImageCache();
}
