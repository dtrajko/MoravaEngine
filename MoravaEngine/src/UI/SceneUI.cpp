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
	if (scene.FindFirst<UIButtonComponent>() != NoEntity && (!m_HasNavigate || !m_HasSubmit))
	{
		Log::GetLogger()->info("UI: the input map has no {0}: the buttons work with the mouse only (Project Settings: Input)",
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
	m_Hovered = m_HoveredButton = m_Selected = m_MousePressed = m_SubmitPressed = NoEntity;
	m_SelectedByMouse = false;
	m_NavigateDirection = glm::ivec2(0);
	m_NavigateRepeat = 0.0f;
	m_Clicks.clear();
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
	const bool overUI = pointer.Inside && HitTest(pointer.Position);
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
	if (!m_Scene)
	{
		return;
	}
	// Laid out again: the fixed steps' scripts may have opened a menu
	UILayout::Build(*m_Scene, m_ViewSize, m_Layout, true);
	auto click = [&](EnvMapVulkanEntityID button) { m_Clicks.push_back({ button }); };

	const UIElementRect* hit = m_Pointer.Inside ? HitTest(m_Pointer.Position) : nullptr;
	m_Hovered = hit ? hit->Entity : NoEntity;
	m_HoveredButton = hit ? FindButton(hit->Entity) : NoEntity;
	if (m_Selected != NoEntity && !IsSelectable(m_Selected))
	{
		m_Selected = NoEntity; // hidden, no longer Interactable, or gone
	}

	// The mouse: moving onto a button selects it, off it drops what the mouse selected; a click is a press and a release
	// over the same button
	const bool hoveringSelectable = m_HoveredButton != NoEntity && IsSelectable(m_HoveredButton);
	if (m_PointerMoved || m_LeftPressed)
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
		m_MousePressed = hoveringSelectable ? m_HoveredButton : NoEntity;
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
	if (input && input->IsEnabled() && m_HasNavigate)
	{
		const glm::vec2 value = input->Get(InputMap::UINavigateAction).Value;
		glm::ivec2 direction(0);
		if (glm::length(value) >= 0.5f)
		{
			// The stronger axis, in the view's directions (y down: up on the stick is -y)
			direction = std::abs(value.x) > std::abs(value.y) ? glm::ivec2(value.x > 0.0f ? 1 : -1, 0) : glm::ivec2(0, value.y > 0.0f ? -1 : 1);
		}
		if (direction != glm::ivec2(0))
		{
			if (direction != m_NavigateDirection)
			{
				Navigate(glm::vec2(direction));
				m_NavigateRepeat = s_NavigateRepeatDelay;
			}
			else if ((m_NavigateRepeat -= unscaledFrameTime) <= 0.0f)
			{
				Navigate(glm::vec2(direction));
				m_NavigateRepeat = s_NavigateRepeatInterval;
			}
		}
		m_NavigateDirection = direction;
	}
	if (input && input->IsEnabled() && m_HasSubmit)
	{
		const InputActionState& submit = input->Get(InputMap::UISubmitAction);
		if (submit.PressedThisFrame && m_Selected != NoEntity)
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

bool SceneUI::Select(EnvMapVulkanEntityID button)
{
	if (button == NoEntity)
	{
		m_Selected = NoEntity;
		return true;
	}
	if (!IsSelectable(button))
	{
		return false;
	}
	m_Selected = button;
	m_SelectedByMouse = false;
	return true;
}

EnvMapVulkanEntityID SceneUI::GetPressed() const
{
	if (m_SubmitPressed != NoEntity)
	{
		return m_SubmitPressed;
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
		const bool catches = m_Scene->Has<UIImageComponent>(it->Entity) || m_Scene->Has<UIButtonComponent>(it->Entity);
		if (catches && point.x >= it->Min.x && point.y >= it->Min.y && point.x < it->Max.x && point.y < it->Max.y)
		{
			return &*it;
		}
	}
	return nullptr;
}

EnvMapVulkanEntityID SceneUI::FindButton(EnvMapVulkanEntityID element) const
{
	for (EnvMapVulkanEntityID id = element; id != NoEntity; id = m_Scene->GetParent(id))
	{
		if (m_Scene->Has<UIButtonComponent>(id))
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

bool SceneUI::IsSelectable(EnvMapVulkanEntityID button) const
{
	if (!m_Scene->Exists(button))
	{
		return false;
	}
	const UIButtonComponent* component = m_Scene->TryGet<UIButtonComponent>(button);
	if (!component || !component->Interactable)
	{
		return false;
	}
	// Shown: under a canvas, no hidden rectangle on the way (from the scene, not this frame's layout: a script may have
	// opened the menu and selected its first button in the same update)
	for (EnvMapVulkanEntityID id = button; id != NoEntity; id = m_Scene->GetParent(id))
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

void SceneUI::Navigate(const glm::vec2& direction)
{
	auto center = [](const UIElementRect& element) { return (element.Min + element.Max) * 0.5f; };
	const UIElementRect* from = m_Selected != NoEntity ? FindElement(m_Selected) : nullptr;

	const UIElementRect* best = nullptr;
	float bestScore = 0.0f;
	for (const UIElementRect& element : m_Layout.Elements)
	{
		if (element.Entity == m_Selected || !m_Scene->Has<UIButtonComponent>(element.Entity) || !IsSelectable(element.Entity))
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

void SceneUI::UpdateTints(float unscaledFrameTime)
{
	const EnvMapVulkanEntityID pressed = GetPressed();
	m_Scene->Each<UIButtonComponent>([&](EnvMapVulkanEntityID entity, UIButtonComponent& button)
	{
		const glm::vec4 target = !button.Interactable ? button.Disabled : entity == pressed ? button.Pressed :
			entity == m_Selected ? button.Highlighted : button.Normal;
		if (!button.TintSet || button.FadeDuration <= 0.0f)
		{
			button.Tint = target;
			button.TintSet = true;
			return;
		}
		// Most of the way there in FadeDuration seconds, whatever the frame rate
		const float k = 1.0f - std::exp(-3.0f * unscaledFrameTime / button.FadeDuration);
		button.Tint += (target - button.Tint) * k;
	});
}
