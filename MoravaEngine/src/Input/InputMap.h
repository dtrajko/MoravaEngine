#pragma once

#include <string>
#include <vector>


// What an action gives the game: a button (held or not, pressed or released this frame), one axis, or two
enum class InputActionType
{
	Button, // Value.x is 0 or 1 (an analog binding, a trigger or a stick, counts as held from InputMap::PressPoint)
	Axis1D, // Value.x, -1 to 1 for keys and sticks (times the binding's Scale)
	Axis2D, // Value.x = right, Value.y = forward (or up)
};

// What a binding reads
enum class InputBindingSource
{
	Key,           // a keyboard key (Code: GLFW_KEY_*)
	Composite,     // keys that make an axis: Up, Down, Left, Right (Axis2D), or Negative and Positive (Axis1D: Left, Right)
	MouseButton,   // Code: GLFW_MOUSE_BUTTON_*
	MouseDelta,    // the mouse's motion, in pixels per second: x right, y up (a rate, as a stick's: times the frame time, it
	               // is the motion this frame, so mouse and stick turn a camera alike: Look * Speed * frameTime)
	MouseWheel,    // the wheel's notches turned this frame (not a rate): x, positive away from the user (a camera's zoom in)
	GamepadButton, // Code: GLFW_GAMEPAD_BUTTON_*
	GamepadAxis,   // one axis of a stick, or a trigger (Code: GLFW_GAMEPAD_AXIS_*; triggers 0 to 1, stick axes up = positive)
	GamepadStick,  // a whole stick (Code: 0 left, 1 right): x right, y up
};

/**
 * One way to give an action a value: a key, keys, a mouse button or motion, a gamepad button, axis or stick. Every connected
 * gamepad counts (the one moved most wins), so a game works with whichever controller is plugged in.
 */
struct InputBinding
{
	InputBindingSource Source = InputBindingSource::Key;
	int Code = -1;                       // the key, button, axis or stick (see InputBindingSource)
	int Up = -1, Down = -1, Left = -1, Right = -1; // Composite: its keys (-1: none)
	float Scale = 1.0f;                  // the value is multiplied by it
	float DeadZone = 0.15f;              // gamepad axes and sticks: below it the value is 0 (the stick at rest drifts a little)
	std::string Text;                    // as the input file names it, for the Input panel: "Key: Space", "Gamepad: LeftStick"
};

struct InputAction
{
	std::string Name;
	InputActionType Type = InputActionType::Button;
	std::vector<InputBinding> Bindings; // all are read; the one with the largest value wins
};

/**
 * The actions of a game and their bindings (docs/engine/Game_Engine_Runtime_Guide.html, 2.1): a project's input file
 * (Settings/Input.minput, YAML). Game code asks for actions ("Jump", "Move"), never for keys, so the keys can change
 * without the game changing, and a gamepad works where a key does:
 *
 *   Actions:
 *     - Name: Move          # Axis2D: x = right, y = forward
 *       Type: Axis2D
 *       Bindings:
 *         - { Composite: WASD, Up: W, Down: S, Left: A, Right: D }
 *         - { Gamepad: LeftStick, DeadZone: 0.15 }
 *     - Name: Jump
 *       Type: Button
 *       Bindings: [ { Key: Space }, { Gamepad: A } ]
 *
 * Bindings: { Key: <key> }, { Composite: <any name>, Up, Down, Left, Right } (Axis1D: Negative, Positive), { Mouse: Left,
 * Right, Middle, Delta or Wheel }, { Gamepad: <button>, <axis>, LeftStick or RightStick }; each can have Scale (and DeadZone for the
 * gamepad's axes). Key names: A to Z, 0 to 9, F1 to F12, Space, Enter, Escape, Tab, Backspace, Up, Down, Left, Right,
 * LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt, Insert, Delete, Home, End, PageUp, PageDown and the
 * keypad's (Keypad0 to Keypad9). Gamepad names (the Xbox layout; a PlayStation controller's Cross is A, Circle B, Square X,
 * Triangle Y): A, B, X, Y, LeftBumper, RightBumper, Back, Start, Guide, LeftThumb, RightThumb, DPadUp, DPadRight, DPadDown,
 * DPadLeft, LeftTrigger, RightTrigger, LeftStickX, LeftStickY, RightStickX, RightStickY, LeftStick, RightStick.
 */
struct InputMap
{
	static constexpr const char* FileExtension = ".minput";

	// A Button action with an analog binding is held from here (a trigger pulled halfway)
	static constexpr float PressPoint = 0.5f;

	std::vector<InputAction> Actions;

	const InputAction* Find(const std::string& name) const;
	// An action is bound to this key (a GLFW_KEY_*), alone or in a composite: the engine then leaves the key to the game
	// (the editor's Escape, the player's Escape out of fullscreen)
	bool BindsKey(int key) const;

	// Move (WASD, the arrow keys, the left stick), Look (the mouse, 0.1 degrees per pixel; the right stick, 180 degrees per
	// second) and Jump (Space, A): what a new project starts with, and what a game without a project gets
	static InputMap Default();

	// Reads an input file. False (error explains) when it can't be read; a binding it doesn't understand is left out, with
	// a warning in the log
	static bool Load(const std::string& filepath, InputMap& map, std::string& error);
	bool Save(const std::string& filepath, std::string& error) const;

	static const char* GetTypeName(InputActionType type);
	// A key's name in input files ("Space"), and the key a name means (-1: none)
	static const char* GetKeyName(int key);
	static int FindKey(const std::string& name);
};
