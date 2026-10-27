#pragma once

#include "InputMap.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>


struct GLFWwindow;

// An action's value and edges (docs/engine/Game_Engine_Runtime_Guide.html, 2.2)
struct InputActionState
{
	glm::vec2 Value = glm::vec2(0.0f); // Button: x = 0 or 1; Axis1D: x
	bool Held = false;                 // a Button held (an axis: moved, its value isn't 0)
	bool PressedThisFrame = false, ReleasedThisFrame = false;   // for the frame's update
	bool PressedSinceFixed = false, ReleasedSinceFixed = false; // for the fixed step: since the last one ran
	int Binding = -1; // the binding that gave the value (the largest), -1: none
};

// A connected controller: a gamepad (GLFW knows its layout, from SDL's database of controllers), or a joystick GLFW has no
// layout for (no binding reads it)
struct InputGamepadInfo
{
	int Index = 0; // GLFW's joystick slot
	std::string Name;
	bool Mapped = false;
};

/**
 * The game's input: the actions of an input map, sampled once per frame from the window's keyboard and mouse and from every
 * connected gamepad. Game code asks for actions by name: Get("Jump").PressedSinceFixed in a fixed step, Get("Move").Value in
 * the frame's update.
 *
 * Edges: a key pressed and released between two fixed steps (a short tap, in a frame without a fixed step) must still reach
 * the fixed step, so the fixed step reads its own flags (...SinceFixed), which collect frames until a fixed step has run
 * (EndFixedStep). The frame's update reads ...ThisFrame.
 *
 * The mouse is captured (the cursor hidden, its motion unlimited) only while the game asks for it and its input is enabled.
 * The game asks for it through an InputTestComponent's CaptureMouse or a script's Input.CursorLocked; ReleaseMouseUntilClick
 * gives it back for a while (the player's Escape), until a click in the window.
 */
class InputSystem
{
public:
	void SetMap(const InputMap& map);
	const InputMap& GetMap() const { return m_Map; }

	// Once per frame, before the game's frame. enabled false: the game gets no input (every action is released), e.g. the
	// editor's viewport isn't focused, or the player's window isn't. frameTime: the frame's real time, for the mouse's rate
	void Update(GLFWwindow* window, bool enabled, float frameTime);

	// After each fixed step (the game loop): its flags start collecting again
	void EndFixedStep();

	// Back to nothing held, and the mouse given back (the game stops)
	void Reset(GLFWwindow* window);

	// An action's state (an action the map doesn't have: nothing held, with a warning once)
	const InputActionState& Get(const std::string& action) const;
	const std::vector<InputActionState>& GetStates() const { return m_States; } // in the map's order

	// The game asks for the mouse: the cursor is hidden and its motion unlimited, while input is enabled
	void SetMouseCaptureWanted(bool wanted) { m_MouseCaptureWanted = wanted; }
	bool IsMouseCaptureWanted() const { return m_MouseCaptureWanted; }
	bool IsMouseCaptured() const { return m_MouseCaptured; }

	// The cursor back to the user while the game still wants the mouse (the player's Escape): captured again at a click in
	// the window
	void ReleaseMouseUntilClick() { m_MouseHeldBack = true; }
	bool IsMouseHeldBack() const { return m_MouseHeldBack; }

	// The wheel's notches (the window's scroll callback, any time): the next Update takes them for MouseWheel bindings
	static void AddMouseWheel(float notches);

	// The game has captured the mouse in this window (the window keeps it captured, not resetting its cursor each frame)
	static bool IsMouseCapturedIn(GLFWwindow* window);

	bool IsEnabled() const { return m_Enabled; }

	static std::vector<InputGamepadInfo> GetGamepads();

private:
	glm::vec2 ReadBinding(GLFWwindow* window, const InputBinding& binding, InputActionType type) const;
	void SetMouseCaptured(GLFWwindow* window, bool captured);

	InputMap m_Map;
	std::vector<InputActionState> m_States;
	bool m_Enabled = false;
	bool m_MouseCaptureWanted = false;
	bool m_MouseCaptured = false;
	bool m_MouseHeldBack = false;
	bool m_HasMousePosition = false;
	glm::dvec2 m_MousePosition = glm::dvec2(0.0);
	glm::vec2 m_MouseRate = glm::vec2(0.0f); // this frame's motion, in pixels per second
	float m_MouseWheel = 0.0f;               // this frame's notches
};
