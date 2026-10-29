// The game UI in the editor (docs/engine/Game_Engine_Runtime_Guide.html, 7.1 to 7.3): adding canvases and elements, the
// Inspector's UI section (canvas, rectangle with anchor presets, text, image, button), and selecting and dragging elements
// in the viewport. The layout is UI/UILayout's, at the viewport's size, as the renderer draws it.


#include "UI/UIFont.h"
#include "UI/UILayout.h"

#include "Project/Project.h"

#include "Scripting/ScriptHost.h"

#include "Core/FileDialogs.h"
#include "Core/ResourceManager.h"
#include "Core/Util.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>


#include "EditorState.h"


static const char* s_FontExtensions = "ttf,otf";
static const char* s_ImageExtensions = "png,jpg,jpeg,tga,bmp";

// The viewport's size in pixels, as the renderer lays the UI out (the Viewport panel's image)
static glm::vec2 s_UIViewSize = glm::vec2(1280.0f, 720.0f);

// An element being dragged in the viewport: where the mouse and the element were when it started
static EnvMapVulkanEntityID s_DraggedElement = NoEntity;
static glm::vec2 s_DragStartMouse = glm::vec2(0.0f);
static glm::vec2 s_DragStartPosition = glm::vec2(0.0f);
static float s_DragScale = 1.0f;

static bool HasExtension(const std::filesystem::path& path, std::initializer_list<const char*> extensions)
{
	std::string extension = path.extension().u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return std::any_of(extensions.begin(), extensions.end(), [&](const char* e) { return extension == e; });
}

static const UIElementRect* FindElementRect(const UIDrawList& list, EnvMapVulkanEntityID entity)
{
	for (const UIElementRect& element : list.Elements)
	{
		if (element.Entity == entity)
		{
			return &element;
		}
	}
	return nullptr;
}

// ---- Adding

// Where a new element goes: under the selected UI entity, else under the scene's first canvas, else under a new canvas
static EnvMapVulkanEntityID GetUIParentForNewElement()
{
	const EnvMapVulkanEntityID selected = s_EditorSelection.GetPrimary();
	const EntityKind kind = s_Scene.Exists(selected) ? GetEntityKind(selected) : EntityKind::Other;
	if (kind == EntityKind::UICanvas || kind == EntityKind::UIElement)
	{
		return selected;
	}
	const EnvMapVulkanEntityID canvas = s_Scene.FindFirst<UICanvasComponent>();
	if (canvas != NoEntity)
	{
		return canvas;
	}
	const EnvMapVulkanEntityID created = s_Scene.CreateEntity("Canvas");
	s_Scene.Add<UICanvasComponent>(created);
	return created;
}

EnvMapVulkanEntityID AddUIEntity(UIEntityPreset preset)
{
	if (preset == UIEntityPreset::Canvas)
	{
		const EnvMapVulkanEntityID canvas = s_Scene.CreateEntity("Canvas");
		s_Scene.Add<UICanvasComponent>(canvas);
		s_EditorSelection.SetPrimary(canvas);
		return canvas;
	}
	const EnvMapVulkanEntityID parent = GetUIParentForNewElement();
	const char* names[] = { "Canvas", "Panel", "Text", "Image", "Button" };
	const EnvMapVulkanEntityID entity = s_Scene.CreateEntity(names[(int)preset], parent);
	UIRectComponent& rect = s_Scene.Add<UIRectComponent>(entity);
	switch (preset)
	{
		case UIEntityPreset::Panel:
			rect.Size = glm::vec2(480.0f, 320.0f);
			s_Scene.Add<UIImageComponent>(entity).Color = glm::vec4(0.05f, 0.05f, 0.08f, 0.7f);
			break;
		case UIEntityPreset::Text:
			rect.Size = glm::vec2(480.0f, 60.0f);
			s_Scene.Add<UITextComponent>(entity);
			break;
		case UIEntityPreset::Image:
			rect.Size = glm::vec2(200.0f, 200.0f);
			s_Scene.Add<UIImageComponent>(entity);
			break;
		case UIEntityPreset::Button:
		{
			// Its image is its background (tinted by its state), its label a stretched child
			rect.Size = glm::vec2(320.0f, 72.0f);
			s_Scene.Add<UIImageComponent>(entity).Color = glm::vec4(0.16f, 0.42f, 0.78f, 1.0f);
			s_Scene.Add<UIButtonComponent>(entity);
			const EnvMapVulkanEntityID label = s_Scene.CreateEntity("Label", entity);
			UIRectComponent& labelRect = s_Scene.Add<UIRectComponent>(label);
			labelRect.AnchorMin = glm::vec2(0.0f);
			labelRect.AnchorMax = glm::vec2(1.0f);
			labelRect.Size = glm::vec2(0.0f);
			UITextComponent& text = s_Scene.Add<UITextComponent>(label);
			text.Text = "Button";
			text.Size = 34.0f;
			break;
		}
		default:
			break;
	}
	s_EditorSelection.SetPrimary(entity);
	return entity;
}

// ---- The Inspector

// A file field: its path (relative to the project), chosen with a file dialog or dropped from the Content Browser; empty
// is allowed (emptyText says what that means)
static bool FileControls(const char* id, const char* label, std::string& path, const char* emptyText, const char* filterName,
	const char* extensions, std::initializer_list<const char*> dottedExtensions, const char* tooltip)
{
	ImGui::PushID(id);
	bool changed = false;
	const std::string shown = path.empty() ? std::string(emptyText) : ProjectPaths::ToStored(path);
	ImGui::SetNextItemWidth(-120.0f);
	ImGui::BeginDisabled(true);
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", shown.c_str());
	ImGui::InputText("##Path", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			std::error_code error;
			const std::filesystem::path dropped = std::filesystem::absolute(std::filesystem::u8path(Util::to_str((const wchar_t*)payload->Data)), error);
			if (HasExtension(dropped, dottedExtensions))
			{
				path = dropped.lexically_normal().generic_u8string();
				changed = true;
			}
			else
			{
				ShowNotification(std::string("Not a file of this kind: ") + extensions, true);
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s\n%s", path.empty() ? emptyText : path.c_str(), tooltip);
	}
	ImGui::SameLine();
	if (ImGui::Button("..."))
	{
		const std::string file = FileDialogs::OpenFile({ { filterName, extensions } }, GetAssetFolderForDialog());
		if (!file.empty())
		{
			path = std::filesystem::u8path(file).lexically_normal().generic_u8string();
			changed = true;
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(path.empty());
	if (ImGui::Button("X"))
	{
		path.clear();
		changed = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextUnformatted(label);
	ImGui::PopID();
	return changed;
}

static void CanvasControls(EnvMapVulkanEntityID entity)
{
	UICanvasComponent& canvas = s_Scene.Get<UICanvasComponent>(entity);
	ImGui::DragFloat2("Reference Resolution", &canvas.ReferenceResolution.x, 1.0f, 1.0f, 16384.0f, "%.0f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The screen size the UI is laid out for, in its own pixels (the elements' Position and Size).\n"
			"On another screen everything is scaled, as Match says.");
	}
	ImGui::SliderFloat("Match", &canvas.MatchWidthOrHeight, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How the UI scales with the screen: 0 = with its width, 1 = with its height,\n"
			"0.5 = in between (a wider or a taller window changes the size equally)");
	}
	ImGui::InputInt("Sort Order", &canvas.SortOrder);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Canvases with a higher number are drawn over the others (a pause menu over the HUD)");
	}
	ImGui::TextDisabled("Scale now: %.3f (viewport %.0f x %.0f)", UILayout::GetCanvasScale(canvas, s_UIViewSize), s_UIViewSize.x, s_UIViewSize.y);
}

// The anchor presets: a 4 x 4 grid (rows: top, middle, bottom, stretched vertically; columns: left, center, right,
// stretched horizontally). Each button draws its anchors in a small parent rectangle
static bool AnchorPresetButton(int row, int column, bool current)
{
	const float size = 26.0f;
	ImGui::PushID(row * 4 + column);
	const bool pressed = ImGui::InvisibleButton("##Preset", ImVec2(size, size));
	const ImVec2 min = ImGui::GetItemRectMin();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImU32 frame = ImGui::GetColorU32(current ? ImGuiCol_ButtonActive : ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	drawList->AddRectFilled(min, ImVec2(min.x + size, min.y + size), frame, 3.0f);
	const ImVec2 innerMin(min.x + 6.0f, min.y + 6.0f);
	const float inner = size - 12.0f;
	drawList->AddRect(innerMin, ImVec2(innerMin.x + inner, innerMin.y + inner), IM_COL32(200, 200, 200, 160));
	const ImU32 anchor = IM_COL32(255, 150, 40, 255);
	const float positions[3] = { 0.0f, 0.5f, 1.0f };
	if (row < 3 && column < 3)
	{
		const ImVec2 point(innerMin.x + positions[column] * inner, innerMin.y + positions[row] * inner);
		drawList->AddRectFilled(ImVec2(point.x - 2.5f, point.y - 2.5f), ImVec2(point.x + 2.5f, point.y + 2.5f), anchor);
	}
	else if (row == 3 && column == 3)
	{
		drawList->AddRect(ImVec2(innerMin.x - 2.0f, innerMin.y - 2.0f), ImVec2(innerMin.x + inner + 2.0f, innerMin.y + inner + 2.0f), anchor, 0.0f, 0, 2.0f);
	}
	else if (row == 3)
	{
		const float x = innerMin.x + positions[column] * inner;
		drawList->AddLine(ImVec2(x, innerMin.y - 3.0f), ImVec2(x, innerMin.y + inner + 3.0f), anchor, 2.0f);
	}
	else
	{
		const float y = innerMin.y + positions[row] * inner;
		drawList->AddLine(ImVec2(innerMin.x - 3.0f, y), ImVec2(innerMin.x + inner + 3.0f, y), anchor, 2.0f);
	}
	ImGui::PopID();
	return pressed;
}

// New anchors (and the pivot that goes with them), keeping the element where it is on the screen: Position and Size are
// worked out again from its current rectangle. moveThere: instead, to the anchor point (no offset), filling the stretched
// directions
static void ApplyAnchorPreset(UIRectComponent& rect, const UIElementRect* placed, glm::vec2 anchorMin, glm::vec2 anchorMax, bool moveThere)
{
	glm::vec2 pivot = rect.Pivot;
	for (int axis = 0; axis < 2; axis++)
	{
		pivot[axis] = anchorMin[axis] == anchorMax[axis] ? anchorMin[axis] : 0.5f;
	}
	if (moveThere || !placed || placed->Scale <= 0.0f)
	{
		for (int axis = 0; axis < 2; axis++)
		{
			const bool stretched = anchorMin[axis] != anchorMax[axis];
			const bool wasStretched = rect.AnchorMin[axis] != rect.AnchorMax[axis];
			if (stretched)
			{
				rect.Size[axis] = 0.0f;
			}
			else if (wasStretched && placed && placed->Scale > 0.0f)
			{
				rect.Size[axis] = (placed->Max[axis] - placed->Min[axis]) / placed->Scale; // the size it had on the screen
			}
		}
		if (moveThere)
		{
			rect.Position = glm::vec2(0.0f);
		}
	}
	else
	{
		const glm::vec2 parentSize = placed->ParentMax - placed->ParentMin;
		const glm::vec2 anchorMinPixels = placed->ParentMin + anchorMin * parentSize;
		const glm::vec2 anchorMaxPixels = placed->ParentMin + anchorMax * parentSize;
		const glm::vec2 sizePixels = placed->Max - placed->Min;
		rect.Size = (sizePixels - (anchorMaxPixels - anchorMinPixels)) / placed->Scale;
		const glm::vec2 pivotPixels = placed->Min + pivot * sizePixels;
		rect.Position = (pivotPixels - (anchorMinPixels + (anchorMaxPixels - anchorMinPixels) * pivot)) / placed->Scale;
	}
	rect.AnchorMin = anchorMin;
	rect.AnchorMax = anchorMax;
	rect.Pivot = pivot;
}

static void RectControls(EnvMapVulkanEntityID entity, const UIDrawList& layout)
{
	UIRectComponent& rect = s_Scene.Get<UIRectComponent>(entity);
	const UIElementRect* placed = FindElementRect(layout, entity);

	ImGui::Checkbox("Visible", &rect.Visible);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Off hides the element and everything under it (a closed menu). Scripts: GetComponent<UIRect>().Visible");
	}

	// The presets
	const float points[3] = { 0.0f, 0.5f, 1.0f };
	ImGui::TextDisabled("Anchors");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Where in its parent the element is held. A point: it keeps its size and stays at Position from\n"
			"that point (a score in the top-left corner stays there in any window). Stretched: it grows with the parent.\n"
			"A preset keeps the element where it is; with Ctrl or Shift it also moves there (and fills the stretched sides).");
	}
	const bool moveThere = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
	for (int row = 0; row < 4; row++)
	{
		for (int column = 0; column < 4; column++)
		{
			const glm::vec2 anchorMin(column < 3 ? points[column] : 0.0f, row < 3 ? points[row] : 0.0f);
			const glm::vec2 anchorMax(column < 3 ? points[column] : 1.0f, row < 3 ? points[row] : 1.0f);
			if (column > 0)
			{
				ImGui::SameLine(0.0f, 3.0f);
			}
			if (AnchorPresetButton(row, column, rect.AnchorMin == anchorMin && rect.AnchorMax == anchorMax))
			{
				ApplyAnchorPreset(rect, placed, anchorMin, anchorMax, moveThere);
			}
			if (ImGui::IsItemHovered())
			{
				const char* rows[4] = { "top", "middle", "bottom", "stretched top to bottom" };
				const char* columns[4] = { "left", "center", "right", "stretched left to right" };
				ImGui::SetTooltip("%s, %s%s", rows[row], columns[column], moveThere ? " (moves there)" : "");
			}
		}
	}

	ImGui::DragFloat2("Anchor Min", &rect.AnchorMin.x, 0.005f, 0.0f, 1.0f, "%.3f");
	ImGui::DragFloat2("Anchor Max", &rect.AnchorMax.x, 0.005f, 0.0f, 1.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Points of the parent's rectangle, (0, 0) top-left to (1, 1) bottom-right. Equal: a point;\n"
			"apart: the element stretches between them");
	}
	ImGui::DragFloat2("Pivot", &rect.Pivot.x, 0.005f, 0.0f, 1.0f, "%.3f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The element's own point that Position places, and that it grows from: (0.5, 0.5) its center");
	}
	ImGui::DragFloat2("Position", &rect.Position.x, 1.0f, 0.0f, 0.0f, "%.1f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("From the anchors to the pivot, in the canvas's reference pixels (x right, y down).\n"
			"Drag the element in the viewport to move it.");
	}
	ImGui::DragFloat2("Size", &rect.Size.x, 1.0f, 0.0f, 0.0f, "%.1f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Reference pixels. Stretched sides: added to the span between the anchors (0: exactly the span,\n"
			"-20: 10 short of each edge)");
	}
	if (placed)
	{
		ImGui::TextDisabled("On the screen: %.0f, %.0f, %.0f x %.0f pixels", placed->Min.x, placed->Min.y, placed->Max.x - placed->Min.x,
			placed->Max.y - placed->Min.y);
	}
	else
	{
		ImGui::TextDisabled(UILayout::FindCanvas(s_Scene, entity) == NoEntity ? "Not drawn: it isn't under a canvas" : "Not drawn: hidden");
	}
}

static void FontAtlasView(const UIFont& font)
{
	if (!ImGui::TreeNode("Font Atlas"))
	{
		return;
	}
	ImGui::TextDisabled("%u x %u texels, %.0f per em, distance range %.0f, %s", font.GetAtlasWidth(), font.GetAtlasHeight(), font.GetEmSize(),
		font.GetDistanceRange(), font.WasMadeNow() ? "made this session" : "from the cache");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Each glyph as a multi-channel signed distance field: every texel holds the distance to the glyph's\n"
			"outline in red, green and blue, each for a different set of its edges. Their median is the true distance,\n"
			"so corners stay sharp. The shader turns the distance into coverage: text is crisp at any size from this one image.\n"
			"Made by msdf-atlas-gen on first use and kept in a cache file (Library/FontCache of the project).");
	}
	if (H2M::RefH2M<H2M::Texture2D_H2M> atlas = font.GetAtlas()) // GetImTextureID registers it with ImGui: not const
	{
		const float width = std::min(ImGui::GetContentRegionAvail().x, (float)font.GetAtlasWidth());
		const float height = width * (float)font.GetAtlasHeight() / (float)std::max(font.GetAtlasWidth(), 1u);
		ImGui::Image((ImTextureID)atlas->GetImTextureID(), ImVec2(width, height));
	}
	ImGui::TreePop();
}

static void TextControls(UITextComponent& text)
{
	char buffer[4096] = {};
	snprintf(buffer, sizeof(buffer), "%s", text.Text.c_str());
	if (ImGui::InputTextMultiline("##Text", buffer, sizeof(buffer), ImVec2(-1.0f, ImGui::GetTextLineHeight() * 3.5f)))
	{
		text.Text = buffer;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The text (Enter starts a new line). Scripts: GetComponent<UIText>().Text = $\"Score {score}\"");
	}
	FileControls("Font", "Font", text.Font, "(the engine's font: Open Sans)", "Font", s_FontExtensions, { ".ttf", ".otf" },
		"A TrueType or OpenType font. Drop one from the Content Browser here, or choose one with ...");
	ImGui::DragFloat("Size##TextSize", &text.Size, 0.25f, 1.0f, 1000.0f, "%.1f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The font's em size in the canvas's reference pixels (capital letters are about 0.7 of it)");
	}
	ImGui::ColorEdit4("Color##TextColor", &text.Color.x);
	int align = (int)text.Align;
	if (ImGui::Combo("Align", &align, "Left\0Center\0Right\0"))
	{
		text.Align = (UITextAlign)align;
	}
	int verticalAlign = (int)text.VerticalAlign;
	if (ImGui::Combo("Vertical Align", &verticalAlign, "Top\0Middle\0Bottom\0"))
	{
		text.VerticalAlign = (UITextVerticalAlign)verticalAlign;
	}
	ImGui::Checkbox("Wrap", &text.Wrap);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Lines are also broken between words where they would leave the rectangle");
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("Line Spacing", &text.LineSpacing, 0.01f, 0.5f, 3.0f, "%.2f");
	if (const std::shared_ptr<UIFont> font = UIFont::Get(text.Font))
	{
		FontAtlasView(*font);
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "The font can't be read (the log says why): nothing is drawn");
	}
}

static void ImageControls(EnvMapVulkanEntityID entity, UIImageComponent& image)
{
	if (FileControls("Texture", "Texture", image.Texture, "(none: a plain rectangle)", "Image", s_ImageExtensions,
		{ ".png", ".jpg", ".jpeg", ".tga", ".bmp" }, "A picture for the rectangle. Drop one from the Content Browser here, or choose one with ..."))
	{
		UILayout::ClearImageCache(); // a file that couldn't be read before is tried again
	}
	ImGui::ColorEdit4("Color##ImageColor", &image.Color.x);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Multiplies the texture (white: as it is); without a texture, the rectangle's color.\n"
			"Alpha below 1 lets the game show through.");
	}
	UIRectComponent* rect = s_Scene.TryGet<UIRectComponent>(entity);
	ImGui::BeginDisabled(image.Texture.empty() || !rect);
	if (ImGui::Button("Native Size"))
	{
		H2M::RefH2M<H2M::Texture2D_H2M> texture = ResourceManager::LoadTexture2D_H2M(image.Texture, false);
		if (texture && texture->Loaded() && rect)
		{
			rect->Size = glm::vec2((float)texture->GetWidth(), (float)texture->GetHeight());
		}
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Size = the texture's own size in pixels (one texel per reference pixel)");
	}
}

static std::string ButtonTargetLabel(EnvMapVulkanEntityID target)
{
	if (target == NoEntity)
	{
		return "(the button, else a parent up to the canvas)";
	}
	const NameComponent* name = s_Scene.TryGet<NameComponent>(target);
	return name ? name->Name : "(missing)";
}

// The entities with a script, in the Scene Hierarchy's order, indented by depth
static void ButtonTargetChoices(const std::vector<EnvMapVulkanEntityID>& entities, int depth, EnvMapVulkanEntityID& target)
{
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (const ScriptComponent* script = s_Scene.TryGet<ScriptComponent>(entity))
		{
			const std::string label = std::string(depth * 2, ' ') + s_Scene.Get<NameComponent>(entity).Name + "  (" + script->ClassName + ")";
			ImGui::PushID((int)(entity & 0x7fffffff));
			if (ImGui::Selectable(label.c_str(), entity == target))
			{
				target = entity;
			}
			ImGui::PopID();
		}
		ButtonTargetChoices(s_Scene.GetChildren(entity), depth + 1, target);
	}
}

static void ButtonControls(EnvMapVulkanEntityID entity, UIButtonComponent& button, const SceneUI* gameUI)
{
	if (gameUI)
	{
		const char* state = !button.Interactable ? "Disabled" : gameUI->GetPressed() == entity ? "Pressed" :
			gameUI->GetSelected() == entity ? "Highlighted (selected)" : "Normal";
		ImGui::TextDisabled("Now: %s", state);
	}
	ImGui::Checkbox("Interactable", &button.Interactable);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Off: the button shows its Disabled color and can't be clicked or selected (C#: button.Interactable)");
	}
	if (!s_Scene.Has<UIImageComponent>(entity))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "No image: the button takes clicks but shows no state (check Image)");
	}
	ImGui::ColorEdit4("Normal", &button.Normal.x);
	ImGui::ColorEdit4("Highlighted", &button.Highlighted.x);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Under the mouse, or selected with the keyboard or the gamepad (UI_Navigate)");
	}
	ImGui::ColorEdit4("Pressed", &button.Pressed.x);
	ImGui::ColorEdit4("Disabled", &button.Disabled.x);
	ImGui::DragFloat("Fade Duration", &button.FadeDuration, 0.005f, 0.0f, 2.0f, "%.2f s");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The colors multiply the image's own color. The change from one to the next takes this long,\n"
			"in real time: a pause menu fades too while the game stands still (time scale 0)");
	}
	// Edit shows the state the game starts in: its tint follows the colors as they are edited
	if (!gameUI)
	{
		button.TintSet = false;
	}

	ImGui::SeparatorText("On Click");
	if (ImGui::BeginCombo("Target", ButtonTargetLabel(button.OnClickTarget).c_str()))
	{
		if (ImGui::Selectable("(the button, else a parent up to the canvas)", button.OnClickTarget == NoEntity))
		{
			button.OnClickTarget = NoEntity;
		}
		ButtonTargetChoices(s_Scene.GetRoots(), 0, button.OnClickTarget);
		ImGui::EndCombo();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VULKAN_SCENE_ENTITY"))
		{
			button.OnClickTarget = *static_cast<const EnvMapVulkanEntityID*>(payload->Data);
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Whose script's method a click calls (an entity can be dropped here from the Scene Hierarchy).\n"
			"None: the first script that has the method, from the button up to its canvas (a menu's script on the canvas)");
	}
	// The method: typed, or one of the target script's (the game's assembly lists only fields, so methods are typed)
	char method[256] = {};
	snprintf(method, sizeof(method), "%s", button.OnClickMethod.c_str());
	if (ImGui::InputText("Method", method, sizeof(method)))
	{
		button.OnClickMethod = method;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A method of the script without parameters, public or not: OnPlay for void OnPlay().\n"
			"Scripts can also listen: GetComponent<UIButton>().Clicked += OnPlay");
	}
	if (button.OnClickTarget != NoEntity && !s_Scene.Exists(button.OnClickTarget))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "The target entity is gone: choose another");
	}
	else if (button.OnClickTarget != NoEntity && !s_Scene.Has<ScriptComponent>(button.OnClickTarget))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "The target has no script");
	}
}

void UIControls(EnvMapVulkanEntityID entity)
{
	const EntityKind kind = GetEntityKind(entity);
	if (kind != EntityKind::UICanvas && kind != EntityKind::UIElement)
	{
		return;
	}
	ImGui::PushID("UI");
	const SceneUI* gameUI = GetGameUI();
	UIDrawList layout;
	UILayout::Build(s_Scene, s_UIViewSize, layout);

	if (kind == EntityKind::UICanvas && ImGui::CollapsingHeader("Canvas", ImGuiTreeNodeFlags_DefaultOpen))
	{
		CanvasControls(entity);
	}
	if (s_Scene.Has<UIRectComponent>(entity) && ImGui::CollapsingHeader("Rectangle", ImGuiTreeNodeFlags_DefaultOpen))
	{
		RectControls(entity, layout);
	}

	// Image and Text: added or removed here (a button is both: its background and its label)
	if (kind == EntityKind::UIElement)
	{
		bool hasImage = s_Scene.Has<UIImageComponent>(entity);
		if (ImGui::Checkbox("Image", &hasImage))
		{
			hasImage ? (void)s_Scene.Add<UIImageComponent>(entity) : s_Scene.Remove<UIImageComponent>(entity);
		}
		ImGui::SameLine();
		bool hasText = s_Scene.Has<UITextComponent>(entity);
		if (ImGui::Checkbox("Text", &hasText))
		{
			hasText ? (void)s_Scene.Add<UITextComponent>(entity) : s_Scene.Remove<UITextComponent>(entity);
		}
		ImGui::SameLine();
		bool hasButton = s_Scene.Has<UIButtonComponent>(entity);
		if (ImGui::Checkbox("Button", &hasButton))
		{
			hasButton ? (void)s_Scene.Add<UIButtonComponent>(entity) : s_Scene.Remove<UIButtonComponent>(entity);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The element takes clicks and the keyboard's and gamepad's selection; its image shows its state");
		}
		if (!s_Scene.Has<UIRectComponent>(entity))
		{
			ImGui::SameLine();
			if (ImGui::SmallButton("Add Rectangle"))
			{
				s_Scene.Add<UIRectComponent>(entity);
			}
		}
	}
	if (UIImageComponent* image = s_Scene.TryGet<UIImageComponent>(entity))
	{
		if (ImGui::CollapsingHeader("Image##Section", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImageControls(entity, *image);
		}
	}
	if (UITextComponent* text = s_Scene.TryGet<UITextComponent>(entity))
	{
		if (ImGui::CollapsingHeader("Text##Section", ImGuiTreeNodeFlags_DefaultOpen))
		{
			TextControls(*text);
		}
	}
	if (UIButtonComponent* button = s_Scene.TryGet<UIButtonComponent>(entity))
	{
		if (ImGui::CollapsingHeader("Button##Section", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ButtonControls(entity, *button, gameUI);
		}
	}
	ImGui::PopID();
}

// ---- The viewport

void SetUIViewSize(const glm::vec2& size)
{
	if (size.x >= 1.0f && size.y >= 1.0f)
	{
		s_UIViewSize = glm::floor(size);
	}
}

// The element under the mouse that shows something (an image or text), the topmost: drawn last
static const UIElementRect* PickElement(const UIDrawList& layout, const glm::vec2& point)
{
	for (auto it = layout.Elements.rbegin(); it != layout.Elements.rend(); ++it)
	{
		const bool shows = s_Scene.Has<UIImageComponent>(it->Entity) || s_Scene.Has<UITextComponent>(it->Entity);
		if (shows && point.x >= it->Min.x && point.y >= it->Min.y && point.x <= it->Max.x && point.y <= it->Max.y)
		{
			return &*it;
		}
	}
	return nullptr;
}

bool HandleViewportUIMouse(const glm::vec2& mouseInView, bool clicked, bool toggle)
{
	// A drag in progress: the element follows the mouse (in its canvas's reference pixels)
	if (s_DraggedElement != NoEntity)
	{
		UIRectComponent* rect = s_Scene.TryGet<UIRectComponent>(s_DraggedElement);
		if (!rect || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			s_DraggedElement = NoEntity;
			return false;
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f))
		{
			rect->Position = glm::round(s_DragStartPosition + (mouseInView - s_DragStartMouse) / s_DragScale);
		}
		return true;
	}
	if (!clicked || !s_ViewOptions.UI || IsGameRunning())
	{
		return false;
	}
	UIDrawList layout;
	UILayout::Build(s_Scene, s_UIViewSize, layout);
	const UIElementRect* picked = PickElement(layout, mouseInView);
	if (!picked)
	{
		return false;
	}
	if (toggle)
	{
		s_EditorSelection.Toggle(picked->Entity);
		return true;
	}
	s_EditorSelection.SelectOnly(picked->Entity);
	if (const UIRectComponent* rect = s_Scene.TryGet<UIRectComponent>(picked->Entity))
	{
		s_DraggedElement = picked->Entity;
		s_DragStartMouse = mouseInView;
		s_DragStartPosition = rect->Position;
		s_DragScale = std::max(picked->Scale, 0.0001f);
	}
	return true;
}
