#pragma once

#include "UILayout.h"

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>


class EnvMapVulkanScene;
class InputSystem;
struct InputMap;

// The mouse as the game's UI sees it: where it is in the view (pixels from the top-left corner, as the UI is laid out),
// and its buttons. Inside false: not over the view, or captured by the game (no cursor), or the game has no input
struct UIPointer
{
	bool Inside = false;
	glm::vec2 Position = glm::vec2(0.0f);
	uint32_t Buttons = 0; // bit i: GLFW_MOUSE_BUTTON_1 + i held down (InputSystem::ReadMouseButtons)
};

/**
 * The game UI's input (docs/engine/Game_Engine_Runtime_Guide.html, 7.3), run by GameSystems while the game runs.
 *
 * The mouse first: a press that begins over an element that catches the mouse (one with an image or a button) belongs to
 * the UI until it is released; InputSystem reads that button as released for the game (UpdatePointer, before the input is
 * sampled), so a click on a button doesn't also fire the gun. A button clicks when it is pressed and released over it.
 *
 * Keyboard and gamepad: UI_Navigate moves the selection to the nearest button or slider in its direction (the first push
 * selects the top-left one, held it repeats), UI_Submit presses the selected button. The mouse moving onto a button or a
 * slider selects it, and moving off it drops a selection the mouse made.
 *
 * Sliders (Milestone 2): a mouse press on one moves its value to the mouse and drags it until the release (also off the
 * slider); selected, UI_Navigate's left and right change its value by its step (held, it repeats), up and down move the
 * selection. Each change the player makes is reported once per frame (GetValueChanges).
 *
 * Real time: the buttons' color fades and the navigation's repeat use the frame's unscaled time, so a pause menu (time
 * scale 0) still works and fades.
 */
class SceneUI
{
public:
	struct Click
	{
		EnvMapVulkanEntityID Button = NoEntity;
	};
	struct ValueChange
	{
		EnvMapVulkanEntityID Slider = NoEntity;
		float Value = 0.0f;
	};

	// The game starts (map: the actions the UI reads, when it has them) or stops
	void Start(EnvMapVulkanScene& scene, const InputMap& map);
	void Stop();
	bool IsRunning() const { return m_Scene != nullptr; }

	// Each frame, before the input is sampled: the mouse over a view of viewSize pixels. Returns the mouse buttons the UI
	// holds (bit i: GLFW_MOUSE_BUTTON_1 + i) for InputSystem::SetBlockedMouseButtons
	uint32_t UpdatePointer(const UIPointer& pointer, const glm::vec2& viewSize);

	// The frame's UI, before the scripts' OnUpdate: the selection, the presses, the clicks (GetClicks), the buttons' colors.
	// input: the game's actions (nullptr: none); unscaledFrameTime: the frame's real time
	void Update(const InputSystem* input, float unscaledFrameTime);
	const std::vector<Click>& GetClicks() const { return m_Clicks; }
	// The sliders the player changed in this frame, each once, with its new value
	const std::vector<ValueChange>& GetValueChanges() const { return m_ValueChanges; }

	// The selected button or slider (NoEntity: none); Select(NoEntity) drops it. False: not a button or slider that can be
	// selected now (it isn't shown, or isn't Interactable)
	EnvMapVulkanEntityID GetSelected() const { return m_Selected; }
	bool Select(EnvMapVulkanEntityID button);
	// The element under the mouse that catches it (NoEntity: none), and whether there is one
	EnvMapVulkanEntityID GetHovered() const { return m_Hovered; }
	bool IsPointerOverUI() const { return m_Hovered != NoEntity; }
	// The button pressed now (by the mouse or by UI_Submit), or the slider the mouse drags; NoEntity: none
	EnvMapVulkanEntityID GetPressed() const;

private:
	const UIElementRect* FindElement(EnvMapVulkanEntityID entity) const;
	// The topmost element under the point that catches the mouse
	const UIElementRect* HitTest(const glm::vec2& point) const;
	// The button or slider an element belongs to: itself, else the nearest ancestor up to its canvas (NoEntity: none)
	EnvMapVulkanEntityID FindSelectable(EnvMapVulkanEntityID element) const;
	bool IsSelectable(EnvMapVulkanEntityID entity) const;
	// UI_Navigate pushed: the selection moves, or a selected slider's value with left and right
	void Navigate(const glm::ivec2& direction);
	void MoveSelection(const glm::vec2& direction);
	// A slider's value from the player (reported once per frame when it changed)
	void SetSliderValue(EnvMapVulkanEntityID entity, float value);
	void DragSlider();
	void UpdateTints(float unscaledFrameTime);

	EnvMapVulkanScene* m_Scene = nullptr;
	bool m_HasNavigate = false, m_HasSubmit = false;
	UIDrawList m_Layout;  // this frame's (UpdatePointer and Update)
	glm::vec2 m_ViewSize = glm::vec2(0.0f);

	UIPointer m_Pointer;
	bool m_PointerMoved = false;
	uint32_t m_PreviousButtons = 0;
	uint32_t m_HeldButtons = 0;      // presses that began over the UI
	bool m_LeftPressed = false, m_LeftReleased = false; // this frame's (UpdatePointer to Update)

	EnvMapVulkanEntityID m_Hovered = NoEntity;
	EnvMapVulkanEntityID m_HoveredButton = NoEntity; // the button or slider m_Hovered belongs to
	EnvMapVulkanEntityID m_Selected = NoEntity;
	bool m_SelectedByMouse = false;
	EnvMapVulkanEntityID m_MousePressed = NoEntity;  // the left button went down on it
	EnvMapVulkanEntityID m_SubmitPressed = NoEntity; // UI_Submit held on it
	EnvMapVulkanEntityID m_DraggedSlider = NoEntity; // the left button went down on it and is held

	glm::ivec2 m_NavigateDirection = glm::ivec2(0); // held now: one of four, or none
	float m_NavigateRepeat = 0.0f;                  // seconds until it moves again

	std::vector<Click> m_Clicks;
	std::vector<ValueChange> m_ValueChanges;
};
