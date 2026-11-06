#include "InputSystem.h"

#include "Core/Log.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <set>


static float s_MouseWheel = 0.0f; // notches since the last Update
static GLFWwindow* s_MouseCapturedWindow = nullptr; // the window the game's mouse is captured in

void InputSystem::AddMouseWheel(float notches)
{
	s_MouseWheel += notches;
}

uint32_t InputSystem::ReadMouseButtons(GLFWwindow* window)
{
	uint32_t buttons = 0;
	for (int button = 0; window && button <= GLFW_MOUSE_BUTTON_LAST && button < 32; button++)
	{
		if (glfwGetMouseButton(window, button) == GLFW_PRESS)
		{
			buttons |= 1u << button;
		}
	}
	return buttons;
}

void InputSystem::SetMap(const InputMap& map)
{
	m_Map = map;
	m_States.assign(m_Map.Actions.size(), InputActionState());
	const bool holding = m_Rebind.Holding;
	m_Rebind = Rebind(); // a rebind's action may not be there any more
	m_Rebind.Holding = holding;
}

// A stick's value with its dead zone: 0 inside it, then 0 to 1 from its edge, so the smallest push past it isn't a jump
static glm::vec2 ApplyDeadZone(glm::vec2 value, float deadZone)
{
	const float length = glm::length(value);
	if (length <= deadZone || length <= 0.0f)
	{
		return glm::vec2(0.0f);
	}
	const float scaled = std::min((length - deadZone) / std::max(1.0f - deadZone, 0.001f), 1.0f);
	return value / length * scaled;
}

static float ApplyDeadZone(float value, float deadZone)
{
	return ApplyDeadZone(glm::vec2(value, 0.0f), deadZone).x;
}

static bool IsKeyDown(GLFWwindow* window, int key)
{
	return key >= 0 && glfwGetKey(window, key) == GLFW_PRESS;
}

glm::vec2 InputSystem::ReadBinding(GLFWwindow* window, const InputBinding& binding, InputActionType type) const
{
	glm::vec2 value(0.0f);
	switch (binding.Source)
	{
		case InputBindingSource::Key:
			value.x = IsKeyDown(window, binding.Code) ? 1.0f : 0.0f;
			break;
		case InputBindingSource::Composite:
			value.x = (IsKeyDown(window, binding.Right) ? 1.0f : 0.0f) - (IsKeyDown(window, binding.Left) ? 1.0f : 0.0f);
			value.y = (IsKeyDown(window, binding.Up) ? 1.0f : 0.0f) - (IsKeyDown(window, binding.Down) ? 1.0f : 0.0f);
			if (glm::length(value) > 1.0f)
			{
				value = glm::normalize(value); // diagonally no faster than straight
			}
			break;
		case InputBindingSource::MouseButton:
			// A press that began over the game's UI is the UI's (a click on a button doesn't also fire the gun)
			value.x = glfwGetMouseButton(window, binding.Code) == GLFW_PRESS && !IsMouseButtonBlocked(binding.Code) ? 1.0f : 0.0f;
			break;
		case InputBindingSource::MouseDelta:
			value = m_MouseRate;
			break;
		case InputBindingSource::MouseWheel:
			value.x = m_MouseWheel;
			break;
		case InputBindingSource::GamepadButton:
		case InputBindingSource::GamepadAxis:
		case InputBindingSource::GamepadStick:
		case InputBindingSource::GamepadDPad:
			// Every gamepad: the one moved most
			for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST; joystick++)
			{
				GLFWgamepadstate state;
				if (!glfwJoystickIsGamepad(joystick) || !glfwGetGamepadState(joystick, &state))
				{
					continue;
				}
				glm::vec2 pad(0.0f);
				if (binding.Source == InputBindingSource::GamepadButton)
				{
					pad.x = state.buttons[binding.Code] == GLFW_PRESS ? 1.0f : 0.0f;
				}
				else if (binding.Source == InputBindingSource::GamepadAxis)
				{
					const bool trigger = binding.Code == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER || binding.Code == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
					const bool vertical = binding.Code == GLFW_GAMEPAD_AXIS_LEFT_Y || binding.Code == GLFW_GAMEPAD_AXIS_RIGHT_Y;
					const float axis = state.axes[binding.Code];
					// Triggers: -1 released to 1 pulled, as 0 to 1; stick axes: GLFW's y is down, ours up
					pad.x = ApplyDeadZone(trigger ? (axis + 1.0f) * 0.5f : vertical ? -axis : axis, binding.DeadZone);
				}
				else if (binding.Source == InputBindingSource::GamepadDPad)
				{
					auto down = [&](int button) { return state.buttons[button] == GLFW_PRESS ? 1.0f : 0.0f; };
					pad = glm::vec2(down(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT) - down(GLFW_GAMEPAD_BUTTON_DPAD_LEFT),
						down(GLFW_GAMEPAD_BUTTON_DPAD_UP) - down(GLFW_GAMEPAD_BUTTON_DPAD_DOWN));
					if (glm::length(pad) > 1.0f)
					{
						pad = glm::normalize(pad);
					}
				}
				else
				{
					const int x = binding.Code == 0 ? GLFW_GAMEPAD_AXIS_LEFT_X : GLFW_GAMEPAD_AXIS_RIGHT_X;
					const int y = binding.Code == 0 ? GLFW_GAMEPAD_AXIS_LEFT_Y : GLFW_GAMEPAD_AXIS_RIGHT_Y;
					pad = ApplyDeadZone(glm::vec2(state.axes[x], -state.axes[y]), binding.DeadZone);
				}
				if (glm::length(pad) > glm::length(value))
				{
					value = pad;
				}
			}
			break;
	}
	value *= binding.Scale;

	// What the action keeps of it: one axis, or two
	if (type != InputActionType::Axis2D)
	{
		// A 2D source on a 1D action: its x (a stick's left-right, a composite's Left and Right)
		value.y = 0.0f;
	}
	return value;
}

void InputSystem::Update(GLFWwindow* window, bool enabled, float frameTime)
{
	m_Enabled = enabled && window;

	// The mouse: captured while the game wants it and has input (and the user didn't take it back: until a click); its
	// motion, as a rate, and the wheel's notches
	auto clicked = [&](int button)
	{
		return glfwGetMouseButton(window, button) == GLFW_PRESS && !IsMouseButtonBlocked(button); // a click on the UI is the UI's
	};
	if (m_MouseHeldBack && (!m_MouseCaptureWanted || (m_Enabled && (clicked(GLFW_MOUSE_BUTTON_LEFT) || clicked(GLFW_MOUSE_BUTTON_RIGHT)))))
	{
		m_MouseHeldBack = false;
	}
	if (window)
	{
		SetMouseCaptured(window, m_Enabled && m_MouseCaptureWanted && !m_MouseHeldBack);
	}
	m_MouseRate = glm::vec2(0.0f);
	m_MouseWheel = m_Enabled ? s_MouseWheel : 0.0f;
	s_MouseWheel = 0.0f;
	if (m_Enabled)
	{
		glm::dvec2 position;
		glfwGetCursorPos(window, &position.x, &position.y);
		if (m_HasMousePosition && frameTime > 0.0f)
		{
			// Pixels per second, y up (the window's y is down)
			m_MouseRate = glm::vec2(float(position.x - m_MousePosition.x), float(m_MousePosition.y - position.y)) / frameTime;
		}
		m_MousePosition = position;
		m_HasMousePosition = true;
	}
	else
	{
		m_HasMousePosition = false; // the first frame with input again starts from where the mouse is then: no jump
	}

	// A rebind listening, or its press still held: the actions read released
	UpdateRebind(window);
	const bool actionsEnabled = m_Enabled && !IsRebinding();

	for (size_t a = 0; a < m_Map.Actions.size() && a < m_States.size(); a++)
	{
		const InputAction& action = m_Map.Actions[a];
		InputActionState& state = m_States[a];

		glm::vec2 value(0.0f);
		int binding = -1;
		if (actionsEnabled)
		{
			for (size_t b = 0; b < action.Bindings.size(); b++)
			{
				const glm::vec2 bindingValue = ReadBinding(window, action.Bindings[b], action.Type);
				if (glm::length(bindingValue) > glm::length(value))
				{
					value = bindingValue;
					binding = (int)b;
				}
			}
		}

		const bool wasHeld = state.Held;
		if (action.Type == InputActionType::Button)
		{
			state.Held = glm::length(value) >= InputMap::PressPoint;
			value = glm::vec2(state.Held ? 1.0f : 0.0f, 0.0f);
		}
		else
		{
			state.Held = value != glm::vec2(0.0f);
		}
		state.Value = value;
		state.Binding = binding;
		state.PressedThisFrame = state.Held && !wasHeld;
		state.ReleasedThisFrame = !state.Held && wasHeld;
		state.PressedSinceFixed |= state.PressedThisFrame;
		state.ReleasedSinceFixed |= state.ReleasedThisFrame;
	}
}

void InputSystem::EndFixedStep()
{
	for (InputActionState& state : m_States)
	{
		state.PressedSinceFixed = false;
		state.ReleasedSinceFixed = false;
	}
}

void InputSystem::Reset(GLFWwindow* window)
{
	if (window)
	{
		SetMouseCaptured(window, false);
	}
	m_States.assign(m_Map.Actions.size(), InputActionState());
	m_Rebind = Rebind();
	m_Enabled = false;
	m_MouseCaptureWanted = false;
	m_MouseHeldBack = false;
	m_BlockedMouseButtons = 0;
	m_HasMousePosition = false;
	m_MouseRate = glm::vec2(0.0f);
	m_MouseWheel = 0.0f;
}

const InputActionState& InputSystem::Get(const std::string& action) const
{
	for (size_t a = 0; a < m_Map.Actions.size() && a < m_States.size(); a++)
	{
		if (m_Map.Actions[a].Name == action)
		{
			return m_States[a];
		}
	}
	static const InputActionState none;
	static std::set<std::string> warned;
	if (warned.insert(action).second)
	{
		Log::GetLogger()->warn("Input: the game asks for the action '{0}', which the input map doesn't have", action);
	}
	return none;
}

bool InputSystem::IsMouseCapturedIn(GLFWwindow* window)
{
	return window && window == s_MouseCapturedWindow;
}

void InputSystem::SetMouseCaptured(GLFWwindow* window, bool captured)
{
	// Again if something else gave the cursor back meanwhile (the editor camera's drag ending)
	if (captured == m_MouseCaptured && (!captured || glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED))
	{
		return;
	}
	s_MouseCapturedWindow = captured ? window : (s_MouseCapturedWindow == window ? nullptr : s_MouseCapturedWindow);
	glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, captured ? GLFW_TRUE : GLFW_FALSE); // no pointer acceleration
	}
	m_MouseCaptured = captured;
	m_HasMousePosition = false; // the cursor's coordinates change with the mode
	Log::GetLogger()->info(captured ? "Input: the mouse is the game's (the cursor hidden)" : "Input: the mouse given back (the cursor shown)");
}

// ---- Rebinding (11.4)

// The keys a rebind takes: those the input files have names for, without Escape (it gives up) and the keys the player keeps
// for itself (F3: the debug overlay, F11: fullscreen)
static bool IsRebindableKey(int key)
{
	return key != GLFW_KEY_ESCAPE && key != GLFW_KEY_F3 && key != GLFW_KEY_F11 && std::string(InputMap::GetKeyName(key)) != "?";
}

// The first gamepad button down on any gamepad (-1: none)
static int FindGamepadButtonDown()
{
	for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST; joystick++)
	{
		GLFWgamepadstate state;
		if (!glfwJoystickIsGamepad(joystick) || !glfwGetGamepadState(joystick, &state))
		{
			continue;
		}
		for (int button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; button++)
		{
			if (state.buttons[button] == GLFW_PRESS)
			{
				return button;
			}
		}
	}
	return -1;
}

// Anything a rebind could take, or give up with, is held
static bool IsAnythingHeld(GLFWwindow* window)
{
	for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++)
	{
		if ((key == GLFW_KEY_ESCAPE || IsRebindableKey(key)) && glfwGetKey(window, key) == GLFW_PRESS)
		{
			return true;
		}
	}
	return InputSystem::ReadMouseButtons(window) != 0 || FindGamepadButtonDown() >= 0;
}

bool InputSystem::StartRebind(const std::string& actionName, int bindingIndex, InputBindingPart part)
{
	for (size_t a = 0; a < m_Map.Actions.size(); a++)
	{
		InputAction& action = m_Map.Actions[a];
		if (action.Name != actionName)
		{
			continue;
		}
		if (bindingIndex < 0 || bindingIndex >= (int)action.Bindings.size())
		{
			return false;
		}
		InputBinding& binding = action.Bindings[bindingIndex];
		const bool composite = binding.Source == InputBindingSource::Composite;
		if (composite != (part != InputBindingPart::Whole))
		{
			return false; // a composite's keys are rebound one by one, and only a composite has parts
		}
		const bool pressable = binding.Source == InputBindingSource::Key || composite || binding.Source == InputBindingSource::MouseButton ||
			binding.Source == InputBindingSource::GamepadButton;
		if (!pressable)
		{
			return false;
		}
		m_Rebind = Rebind();
		m_Rebind.State = InputRebindState::Listening;
		m_Rebind.Action = (int)a;
		m_Rebind.Binding = bindingIndex;
		m_Rebind.Part = part;
		m_Rebind.Gamepad = binding.Source == InputBindingSource::GamepadButton;
		m_Rebind.WaitRelease = true;
		Log::GetLogger()->info("Input: rebinding '{0}' ({1}): waiting for a {2}", action.Name, InputMap::DescribeBinding(binding, part),
			m_Rebind.Gamepad ? "gamepad button" : composite ? "key" : "key or mouse button");
		return true;
	}
	return false;
}

void InputSystem::CancelRebind()
{
	if (m_Rebind.State == InputRebindState::Listening)
	{
		m_Rebind.State = InputRebindState::Cancelled;
		m_Rebind.Holding = true; // what gave up (a click on Back) is released before the actions count again
		Log::GetLogger()->info("Input: rebinding given up");
	}
}

bool InputSystem::TakeRebound(std::string& action)
{
	if (!m_Rebind.Rebound || m_Rebind.Action < 0 || m_Rebind.Action >= (int)m_Map.Actions.size())
	{
		return false;
	}
	m_Rebind.Rebound = false;
	action = m_Map.Actions[m_Rebind.Action].Name;
	return true;
}

void InputSystem::UpdateRebind(GLFWwindow* window)
{
	if (!window)
	{
		return;
	}
	if (m_Rebind.Holding)
	{
		m_Rebind.Holding = IsAnythingHeld(window);
		return;
	}
	if (m_Rebind.State != InputRebindState::Listening || !m_Enabled)
	{
		return; // without the window's focus nothing is pressed for it
	}
	if (m_Rebind.WaitRelease)
	{
		m_Rebind.WaitRelease = IsAnythingHeld(window);
		return;
	}

	InputBinding& binding = m_Map.Actions[m_Rebind.Action].Bindings[m_Rebind.Binding];
	const int gamepadButton = FindGamepadButtonDown();
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || gamepadButton == GLFW_GAMEPAD_BUTTON_BACK)
	{
		CancelRebind();
		return;
	}

	// What was pressed, as the binding it becomes
	InputBinding pressed = binding;
	bool found = false;
	if (m_Rebind.Gamepad)
	{
		if (gamepadButton >= 0)
		{
			pressed.Source = InputBindingSource::GamepadButton;
			pressed.Code = gamepadButton;
			found = true;
		}
	}
	else
	{
		for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST && !found; key++)
		{
			if (IsRebindableKey(key) && glfwGetKey(window, key) == GLFW_PRESS)
			{
				if (int* part = InputMap::GetCompositeKey(pressed, m_Rebind.Part))
				{
					*part = key;
				}
				else
				{
					pressed.Source = InputBindingSource::Key;
					pressed.Code = key;
				}
				found = true;
			}
		}
		for (int button = GLFW_MOUSE_BUTTON_LEFT; button <= GLFW_MOUSE_BUTTON_MIDDLE && !found && m_Rebind.Part == InputBindingPart::Whole; button++)
		{
			if (glfwGetMouseButton(window, button) == GLFW_PRESS)
			{
				pressed.Source = InputBindingSource::MouseButton;
				pressed.Code = button;
				found = true;
			}
		}
	}
	if (!found)
	{
		return;
	}

	// The text the Input panel shows ("Key: J"), as an input file would have it
	switch (pressed.Source)
	{
		case InputBindingSource::Key:           pressed.Text = std::string("Key: ") + InputMap::GetKeyName(pressed.Code); break;
		case InputBindingSource::MouseButton:   pressed.Text = InputMap::DescribeBinding(pressed); pressed.Text.replace(0, 6, "Mouse: "); break;
		case InputBindingSource::GamepadButton: pressed.Text = InputMap::DescribeBinding(pressed); pressed.Text.replace(0, 8, "Gamepad: "); break;
		default: break; // a composite keeps its name
	}
	Log::GetLogger()->info("Input: '{0}' {1}: {2} instead of {3}", m_Map.Actions[m_Rebind.Action].Name,
		m_Rebind.Part == InputBindingPart::Whole ? "binding " + std::to_string(m_Rebind.Binding + 1) : "key",
		InputMap::DescribeBinding(pressed, m_Rebind.Part), InputMap::DescribeBinding(binding, m_Rebind.Part));
	binding = pressed;
	m_Rebind.State = InputRebindState::Done;
	m_Rebind.Holding = true;
	m_Rebind.Rebound = true;
}

std::vector<InputGamepadInfo> InputSystem::GetGamepads()
{
	std::vector<InputGamepadInfo> gamepads;
	for (int joystick = GLFW_JOYSTICK_1; joystick <= GLFW_JOYSTICK_LAST; joystick++)
	{
		if (!glfwJoystickPresent(joystick))
		{
			continue;
		}
		InputGamepadInfo info;
		info.Index = joystick;
		info.Mapped = glfwJoystickIsGamepad(joystick);
		const char* name = info.Mapped ? glfwGetGamepadName(joystick) : glfwGetJoystickName(joystick);
		info.Name = name ? name : "?";
		gamepads.push_back(info);
	}
	return gamepads;
}
