#include "SceneUI.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Core/Log.h"

#include "Input/InputSystem.h"

#include <cmath>


// UI_Navigate held: the selection moves again after this long, then at this interval (seconds of real time)
static constexpr float s_NavigateRepeatDelay = 0.4f;
static constexpr float s_NavigateRepeatInterval = 0.12f;

void SceneUI::Start(EnvMapVulkanScene& scene, const InputMap& map)
{
	Stop();
	m_Scene = &scene;
	m_HasNavigate = map.Find(InputMap::UINavigateAction) != nullptr;
	m_HasSubmit = map.Find(InputMap::UISubmitAction) != nullptr;
	// No list open from an earlier run of the scene
	scene.Each<UIDropdownComponent>([](EnvMapVulkanEntityID, UIDropdownComponent& dropdown) { dropdown.Expanded = false; });
	const bool selectables = scene.FindFirst<UIButtonComponent>() != NoEntity || scene.FindFirst<UISliderComponent>() != NoEntity ||
		scene.FindFirst<UIDropdownComponent>() != NoEntity;
	if (selectables && (!m_HasNavigate || !m_HasSubmit))
	{
		Log::GetLogger()->info("UI: the input map has no {0}: the buttons, sliders and drop-downs work with the mouse only (Project Settings: Input)",
			!m_HasNavigate && !m_HasSubmit ? "UI_Navigate and UI_Submit" : !m_HasNavigate ? "UI_Navigate" : "UI_Submit");
	}
}

void SceneUI::Stop()
{
	m_Scene = nullptr;
	m_Layout.Clear();
	m_Pointer = UIPointer();
	m_PointerMoved = false;
	m_PreviousButtons = m_HeldButtons = 0;
	m_LeftPressed = m_LeftReleased = false;
	m_Hovered = m_HoveredButton = m_Selected = m_MousePressed = m_SubmitPressed = m_DraggedSlider = m_OpenDropdown = NoEntity;
	m_DropdownPressedRow = -1;
	m_SelectedByMouse = false;
	m_NavigateDirection = glm::ivec2(0);
	m_NavigateRepeat = 0.0f;
	m_Clicks.clear();
	m_ValueChanges.clear();
	m_DropdownChanges.clear();
}

uint32_t SceneUI::UpdatePointer(const UIPointer& pointer, const glm::vec2& viewSize)
{
	if (!m_Scene)
	{
		return 0;
	}
	m_ViewSize = viewSize;
	UILayout::Build(*m_Scene, viewSize, m_Layout, true);

	m_PointerMoved = pointer.Inside != m_Pointer.Inside || (pointer.Inside && pointer.Position != m_Pointer.Position);
	// An open drop-down's list has every press (one outside it closes it)
	const bool overUI = pointer.Inside && (m_OpenDropdown != NoEntity || HitTest(pointer.Position));
	const uint32_t pressed = pointer.Buttons & ~m_PreviousButtons;
	const uint32_t released = m_PreviousButtons & ~pointer.Buttons;
	// A press that begins over the UI is the UI's until its release
	m_HeldButtons = (m_HeldButtons & pointer.Buttons) | (overUI ? pressed : 0u);
	m_LeftPressed |= (pressed & 1u) != 0;
	m_LeftReleased |= (released & 1u) != 0;
	m_PreviousButtons = pointer.Buttons;
	m_Pointer = pointer;
	return m_HeldButtons;
}

void SceneUI::Update(const InputSystem* input, float unscaledFrameTime)
{
	m_Clicks.clear();
	m_ValueChanges.clear();
	m_DropdownChanges.clear();
	if (!m_Scene)
	{
		return;
	}
	// Laid out again: the fixed steps' scripts may have opened a menu
	UILayout::Build(*m_Scene, m_ViewSize, m_Layout, true);
	if (m_OpenDropdown != NoEntity && !IsSelectable(m_OpenDropdown))
	{
		CloseDropdown(); // hidden, no longer Interactable, or gone
	}
	if (m_OpenDropdown != NoEntity)
	{
		UpdateOpenDropdown(input, unscaledFrameTime);
		UpdateTints(unscaledFrameTime);
		return;
	}
	// A click: a button's, or a drop-down opens
	auto click = [&](EnvMapVulkanEntityID element)
	{
		if (m_Scene->Has<UIDropdownComponent>(element))
		{
			OpenDropdown(element);
		}
		else
		{
			m_Clicks.push_back({ element });
		}
	};

	const UIElementRect* hit = m_Pointer.Inside ? HitTest(m_Pointer.Position) : nullptr;
	m_Hovered = hit ? hit->Entity : NoEntity;
	m_HoveredButton = hit ? FindSelectable(hit->Entity) : NoEntity;
	if (m_Selected != NoEntity && !IsSelectable(m_Selected))
	{
		m_Selected = NoEntity; // hidden, no longer Interactable, or gone
	}
	if (m_DraggedSlider != NoEntity && !IsSelectable(m_DraggedSlider))
	{
		m_DraggedSlider = NoEntity;
	}

	// The mouse: moving onto a button or slider selects it, off it drops what the mouse selected (not while it drags a
	// slider); a click is a press and a release over the same button; a press on a slider drags it
	const bool hoveringSelectable = m_HoveredButton != NoEntity && IsSelectable(m_HoveredButton);
	if ((m_PointerMoved || m_LeftPressed) && m_DraggedSlider == NoEntity)
	{
		if (hoveringSelectable)
		{
			m_Selected = m_HoveredButton;
			m_SelectedByMouse = true;
		}
		else if (m_SelectedByMouse)
		{
			m_Selected = NoEntity;
		}
	}
	if (m_LeftPressed)
	{
		const bool slider = hoveringSelectable && m_Scene->Has<UISliderComponent>(m_HoveredButton);
		m_MousePressed = hoveringSelectable && !slider ? m_HoveredButton : NoEntity;
		m_DraggedSlider = slider ? m_HoveredButton : NoEntity;
	}
	if (m_DraggedSlider != NoEntity)
	{
		DragSlider();
	}
	if (m_LeftReleased || !(m_Pointer.Buttons & 1u))
	{
		m_DraggedSlider = NoEntity;
	}
	if (m_LeftReleased)
	{
		if (m_MousePressed != NoEntity && m_MousePressed == m_HoveredButton && hoveringSelectable)
		{
			click(m_MousePressed);
		}
		m_MousePressed = NoEntity;
	}
	m_LeftPressed = m_LeftReleased = false;
	m_PointerMoved = false;

	// The keyboard and the gamepad
	glm::ivec2 direction(0);
	if (ReadNavigate(input, unscaledFrameTime, direction))
	{
		Navigate(direction);
	}
	if (input && input->IsEnabled() && m_HasSubmit)
	{
		const InputActionState& submit = input->Get(InputMap::UISubmitAction);
		const bool pressable = m_Selected != NoEntity && (m_Scene->Has<UIButtonComponent>(m_Selected) || m_Scene->Has<UIDropdownComponent>(m_Selected));
		if (submit.PressedThisFrame && pressable)
		{
			click(m_Selected);
			m_SubmitPressed = m_Selected;
		}
		if (!submit.Held)
		{
			m_SubmitPressed = NoEntity;
		}
	}
	UpdateTints(unscaledFrameTime);
}

bool SceneUI::ReadNavigate(const InputSystem* input, float unscaledFrameTime, glm::ivec2& direction)
{
	direction = glm::ivec2(0);
	if (!input || !input->IsEnabled() || !m_HasNavigate)
	{
		return false;
	}
	const glm::vec2 value = input->Get(InputMap::UINavigateAction).Value;
	if (glm::length(value) >= 0.5f)
	{
		// The stronger axis, in the view's directions (y down: up on the stick is -y)
		direction = std::abs(value.x) > std::abs(value.y) ? glm::ivec2(value.x > 0.0f ? 1 : -1, 0) : glm::ivec2(0, value.y > 0.0f ? -1 : 1);
	}
	bool moves = false;
	if (direction != glm::ivec2(0))
	{
		if (direction != m_NavigateDirection)
		{
			moves = true;
			m_NavigateRepeat = s_NavigateRepeatDelay;
		}
		else if ((m_NavigateRepeat -= unscaledFrameTime) <= 0.0f)
		{
			moves = true;
			m_NavigateRepeat = s_NavigateRepeatInterval;
		}
	}
	m_NavigateDirection = direction;
	return moves;
}

bool SceneUI::OpenDropdown(EnvMapVulkanEntityID entity)
{
	UIDropdownComponent* dropdown = m_Scene ? m_Scene->TryGet<UIDropdownComponent>(entity) : nullptr;
	if (!dropdown || dropdown->Options.empty() || !IsSelectable(entity))
	{
		return false;
	}
	if (m_OpenDropdown != entity)
	{
		CloseDropdown();
	}
	m_OpenDropdown = entity;
	dropdown->Expanded = true;
	dropdown->HighlightedOption = dropdown->Constrain(dropdown->Value);
	m_Selected = entity;
	m_MousePressed = m_SubmitPressed = NoEntity;
	m_DropdownPressedRow = -1;
	return true;
}

void SceneUI::CloseDropdown()
{
	if (m_Scene && m_OpenDropdown != NoEntity)
	{
		if (UIDropdownComponent* dropdown = m_Scene->TryGet<UIDropdownComponent>(m_OpenDropdown))
		{
			dropdown->Expanded = false;
		}
	}
	m_OpenDropdown = NoEntity;
	m_DropdownPressedRow = -1;
}

void SceneUI::ChooseOption(int option)
{
	UIDropdownComponent* dropdown = m_Scene->TryGet<UIDropdownComponent>(m_OpenDropdown);
	if (dropdown && !dropdown->Options.empty())
	{
		option = dropdown->Constrain(option);
		const bool changed = option != dropdown->Constrain(dropdown->Value);
		dropdown->Value = option;
		if (changed)
		{
			m_DropdownChanges.push_back({ m_OpenDropdown, option });
		}
	}
	// The selection stays on the drop-down: the keyboard goes on from there
	m_Selected = m_OpenDropdown;
	m_SelectedByMouse = false;
	CloseDropdown();
}

void SceneUI::UpdateOpenDropdown(const InputSystem* input, float unscaledFrameTime)
{
	UIDropdownComponent* dropdown = m_Scene->TryGet<UIDropdownComponent>(m_OpenDropdown);
	const UIElementRect* element = FindElement(m_OpenDropdown);
	if (!dropdown || !element || dropdown->Options.empty())
	{
		CloseDropdown();
		return;
	}
	const UIDropdownListGeometry list = UILayout::GetDropdownList(*dropdown, element->Min, element->Max, m_ViewSize);
	const glm::vec2 point = m_Pointer.Position;
	const int row = m_Pointer.Inside ? list.RowAt(point) : -1;
	const bool onElement = m_Pointer.Inside && point.x >= element->Min.x && point.y >= element->Min.y && point.x < element->Max.x &&
		point.y < element->Max.y;
	m_Hovered = m_HoveredButton = row >= 0 || onElement ? m_OpenDropdown : NoEntity;
	dropdown->HighlightedOption = dropdown->Constrain(dropdown->HighlightedOption);

	// The mouse: the row under it is highlighted; a press and a release on a row chooses it; a click on the drop-down
	// itself closes the list, a press anywhere else closes it at once (and does nothing more)
	if (m_PointerMoved && row >= 0)
	{
		dropdown->HighlightedOption = row;
	}
	const EnvMapVulkanEntityID open = m_OpenDropdown;
	if (m_LeftPressed)
	{
		m_DropdownPressedRow = row >= 0 ? row : onElement ? -2 : -1;
		if (row < 0 && !onElement)
		{
			CloseDropdown();
		}
	}
	if (m_LeftReleased && m_OpenDropdown == open)
	{
		if (m_DropdownPressedRow >= 0 && row == m_DropdownPressedRow)
		{
			ChooseOption(row);
		}
		else if (m_DropdownPressedRow == -2 && onElement)
		{
			CloseDropdown();
		}
		m_DropdownPressedRow = -1;
	}
	m_LeftPressed = m_LeftReleased = false;
	m_PointerMoved = false;

	// The keyboard and the gamepad: up and down along the list (held, it repeats), UI_Submit chooses
	glm::ivec2 direction(0);
	if (ReadNavigate(input, unscaledFrameTime, direction) && direction.y != 0 && m_OpenDropdown == open)
	{
		dropdown->HighlightedOption = dropdown->Constrain(dropdown->HighlightedOption + direction.y);
	}
	if (input && input->IsEnabled() && m_HasSubmit)
	{
		const InputActionState& submit = input->Get(InputMap::UISubmitAction);
		if (submit.PressedThisFrame && m_OpenDropdown == open)
		{
			ChooseOption(dropdown->HighlightedOption);
			m_SubmitPressed = open;
		}
		if (!submit.Held)
		{
			m_SubmitPressed = NoEntity;
		}
	}
}

bool SceneUI::Select(EnvMapVulkanEntityID entity)
{
	if (entity == NoEntity)
	{
		m_Selected = NoEntity;
		return true;
	}
	if (!IsSelectable(entity))
	{
		return false;
	}
	m_Selected = entity;
	m_SelectedByMouse = false;
	return true;
}

EnvMapVulkanEntityID SceneUI::GetPressed() const
{
	if (m_SubmitPressed != NoEntity)
	{
		return m_SubmitPressed;
	}
	if (m_DraggedSlider != NoEntity)
	{
		return m_DraggedSlider;
	}
	return m_MousePressed != NoEntity && m_MousePressed == m_HoveredButton ? m_MousePressed : NoEntity;
}

const UIElementRect* SceneUI::FindElement(EnvMapVulkanEntityID entity) const
{
	for (const UIElementRect& element : m_Layout.Elements)
	{
		if (element.Entity == entity)
		{
			return &element;
		}
	}
	return nullptr;
}

const UIElementRect* SceneUI::HitTest(const glm::vec2& point) const
{
	// The topmost: drawn last
	for (auto it = m_Layout.Elements.rbegin(); it != m_Layout.Elements.rend(); ++it)
	{
		const bool catches = m_Scene->Has<UIImageComponent>(it->Entity) || m_Scene->Has<UIButtonComponent>(it->Entity) ||
			m_Scene->Has<UISliderComponent>(it->Entity) || m_Scene->Has<UIDropdownComponent>(it->Entity);
		if (catches && point.x >= it->Min.x && point.y >= it->Min.y && point.x < it->Max.x && point.y < it->Max.y)
		{
			return &*it;
		}
	}
	return nullptr;
}

EnvMapVulkanEntityID SceneUI::FindSelectable(EnvMapVulkanEntityID element) const
{
	for (EnvMapVulkanEntityID id = element; id != NoEntity; id = m_Scene->GetParent(id))
	{
		if (m_Scene->Has<UIButtonComponent>(id) || m_Scene->Has<UISliderComponent>(id) || m_Scene->Has<UIDropdownComponent>(id))
		{
			return id;
		}
		if (m_Scene->Has<UICanvasComponent>(id))
		{
			break;
		}
	}
	return NoEntity;
}

bool SceneUI::IsSelectable(EnvMapVulkanEntityID entity) const
{
	if (!m_Scene->Exists(entity))
	{
		return false;
	}
	const UIButtonComponent* button = m_Scene->TryGet<UIButtonComponent>(entity);
	const UISliderComponent* slider = m_Scene->TryGet<UISliderComponent>(entity);
	const UIDropdownComponent* dropdown = m_Scene->TryGet<UIDropdownComponent>(entity);
	if (!(button ? button->Interactable : slider ? slider->Interactable : dropdown && dropdown->Interactable))
	{
		return false;
	}
	// Shown: under a canvas, no hidden rectangle on the way (from the scene, not this frame's layout: a script may have
	// opened the menu and selected its first button in the same update)
	for (EnvMapVulkanEntityID id = entity; id != NoEntity; id = m_Scene->GetParent(id))
	{
		const UIRectComponent* rect = m_Scene->TryGet<UIRectComponent>(id);
		if (rect && !rect->Visible)
		{
			return false;
		}
		if (m_Scene->Has<UICanvasComponent>(id))
		{
			return true;
		}
	}
	return false;
}

void SceneUI::Navigate(const glm::ivec2& direction)
{
	// A selected slider takes left and right: its value moves by its step, towards Max on the right
	if (direction.x != 0 && m_Selected != NoEntity)
	{
		if (const UISliderComponent* slider = m_Scene->TryGet<UISliderComponent>(m_Selected))
		{
			const float sign = slider->Max >= slider->Min ? 1.0f : -1.0f;
			SetSliderValue(m_Selected, slider->Constrain(slider->Value) + float(direction.x) * sign * slider->GetStep());
			return;
		}
	}
	MoveSelection(glm::vec2(direction));
}

void SceneUI::MoveSelection(const glm::vec2& direction)
{
	auto center = [](const UIElementRect& element) { return (element.Min + element.Max) * 0.5f; };
	const UIElementRect* from = m_Selected != NoEntity ? FindElement(m_Selected) : nullptr;

	const UIElementRect* best = nullptr;
	float bestScore = 0.0f;
	for (const UIElementRect& element : m_Layout.Elements)
	{
		if (element.Entity == m_Selected || !IsSelectable(element.Entity))
		{
			continue;
		}
		float score = 0.0f;
		if (from)
		{
			// Ahead in the direction, nearest, and off its line counting double: the button below, not the one below and aside
			const glm::vec2 offset = center(element) - center(*from);
			const float along = glm::dot(offset, direction);
			if (along < 1.0f)
			{
				continue;
			}
			score = along + 2.0f * glm::length(offset - along * direction);
		}
		else
		{
			// Nothing selected: the top-left button (a menu's first)
			const glm::vec2 c = center(element);
			score = c.y * 4.0f + c.x;
		}
		if (!best || score < bestScore)
		{
			best = &element;
			bestScore = score;
		}
	}
	if (best)
	{
		m_Selected = best->Entity;
		m_SelectedByMouse = false;
	}
}

void SceneUI::SetSliderValue(EnvMapVulkanEntityID entity, float value)
{
	UISliderComponent* slider = m_Scene->TryGet<UISliderComponent>(entity);
	if (!slider)
	{
		return;
	}
	value = slider->Constrain(value);
	if (value == slider->Value)
	{
		return;
	}
	slider->Value = value;
	for (ValueChange& change : m_ValueChanges)
	{
		if (change.Slider == entity)
		{
			change.Value = value; // once per frame, with the last value
			return;
		}
	}
	m_ValueChanges.push_back({ entity, value });
}

void SceneUI::DragSlider()
{
	const UISliderComponent* slider = m_Scene->TryGet<UISliderComponent>(m_DraggedSlider);
	const UIElementRect* element = FindElement(m_DraggedSlider);
	if (!slider || !element)
	{
		return;
	}
	// The mouse's x along the handle's travel (beyond its ends: the end)
	const UISliderGeometry geometry = UILayout::GetSliderGeometry(*slider, element->Min, element->Max, element->Scale);
	const float travel = geometry.TravelMax - geometry.TravelMin;
	const float t = travel > 0.0f ? glm::clamp((m_Pointer.Position.x - geometry.TravelMin) / travel, 0.0f, 1.0f) : 0.0f;
	SetSliderValue(m_DraggedSlider, slider->Min + (slider->Max - slider->Min) * t);
}

// A button's or a slider's color of its state, faded over its FadeDuration
template<typename T>
static void FadeTint(T& component, bool pressed, bool selected, float unscaledFrameTime)
{
	const glm::vec4 target = !component.Interactable ? component.Disabled : pressed ? component.Pressed :
		selected ? component.Highlighted : component.Normal;
	if (!component.TintSet || component.FadeDuration <= 0.0f)
	{
		component.Tint = target;
		component.TintSet = true;
		return;
	}
	// Most of the way there in FadeDuration seconds, whatever the frame rate
	const float k = 1.0f - std::exp(-3.0f * unscaledFrameTime / component.FadeDuration);
	component.Tint += (target - component.Tint) * k;
}

void SceneUI::UpdateTints(float unscaledFrameTime)
{
	const EnvMapVulkanEntityID pressed = GetPressed();
	m_Scene->Each<UIButtonComponent>([&](EnvMapVulkanEntityID entity, UIButtonComponent& button)
	{
		FadeTint(button, entity == pressed, entity == m_Selected, unscaledFrameTime);
	});
	m_Scene->Each<UISliderComponent>([&](EnvMapVulkanEntityID entity, UISliderComponent& slider)
	{
		FadeTint(slider, entity == pressed, entity == m_Selected, unscaledFrameTime);
	});
	m_Scene->Each<UIDropdownComponent>([&](EnvMapVulkanEntityID entity, UIDropdownComponent& dropdown)
	{
		FadeTint(dropdown, entity == pressed, entity == m_Selected || entity == m_OpenDropdown, unscaledFrameTime);
	});
}
