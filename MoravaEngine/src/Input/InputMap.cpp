#include "InputMap.h"

#include "Core/Log.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <utility>


// The names input files use: keys, mouse buttons, gamepad buttons and axes
struct InputName
{
	const char* Name;
	int Code;
};

static const InputName s_KeyNames[] =
{
	{ "Space", GLFW_KEY_SPACE }, { "Enter", GLFW_KEY_ENTER }, { "Escape", GLFW_KEY_ESCAPE }, { "Tab", GLFW_KEY_TAB },
	{ "Backspace", GLFW_KEY_BACKSPACE }, { "Up", GLFW_KEY_UP }, { "Down", GLFW_KEY_DOWN }, { "Left", GLFW_KEY_LEFT },
	{ "Right", GLFW_KEY_RIGHT }, { "LeftShift", GLFW_KEY_LEFT_SHIFT }, { "RightShift", GLFW_KEY_RIGHT_SHIFT },
	{ "LeftControl", GLFW_KEY_LEFT_CONTROL }, { "RightControl", GLFW_KEY_RIGHT_CONTROL }, { "LeftAlt", GLFW_KEY_LEFT_ALT },
	{ "RightAlt", GLFW_KEY_RIGHT_ALT }, { "Insert", GLFW_KEY_INSERT }, { "Delete", GLFW_KEY_DELETE }, { "Home", GLFW_KEY_HOME },
	{ "End", GLFW_KEY_END }, { "PageUp", GLFW_KEY_PAGE_UP }, { "PageDown", GLFW_KEY_PAGE_DOWN },
	{ "Minus", GLFW_KEY_MINUS }, { "Equal", GLFW_KEY_EQUAL }, { "Comma", GLFW_KEY_COMMA }, { "Period", GLFW_KEY_PERIOD },
	{ "Slash", GLFW_KEY_SLASH }, { "Semicolon", GLFW_KEY_SEMICOLON }, { "Apostrophe", GLFW_KEY_APOSTROPHE },
	{ "LeftBracket", GLFW_KEY_LEFT_BRACKET }, { "RightBracket", GLFW_KEY_RIGHT_BRACKET }, { "Backslash", GLFW_KEY_BACKSLASH },
	{ "GraveAccent", GLFW_KEY_GRAVE_ACCENT }, { "KeypadEnter", GLFW_KEY_KP_ENTER }, { "KeypadAdd", GLFW_KEY_KP_ADD },
	{ "KeypadSubtract", GLFW_KEY_KP_SUBTRACT },
};

static const InputName s_MouseNames[] =
{
	{ "Left", GLFW_MOUSE_BUTTON_LEFT }, { "Right", GLFW_MOUSE_BUTTON_RIGHT }, { "Middle", GLFW_MOUSE_BUTTON_MIDDLE },
};

static const InputName s_GamepadButtonNames[] =
{
	{ "A", GLFW_GAMEPAD_BUTTON_A }, { "B", GLFW_GAMEPAD_BUTTON_B }, { "X", GLFW_GAMEPAD_BUTTON_X }, { "Y", GLFW_GAMEPAD_BUTTON_Y },
	{ "LeftBumper", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER }, { "RightBumper", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER },
	{ "Back", GLFW_GAMEPAD_BUTTON_BACK }, { "Start", GLFW_GAMEPAD_BUTTON_START }, { "Guide", GLFW_GAMEPAD_BUTTON_GUIDE },
	{ "LeftThumb", GLFW_GAMEPAD_BUTTON_LEFT_THUMB }, { "RightThumb", GLFW_GAMEPAD_BUTTON_RIGHT_THUMB },
	{ "DPadUp", GLFW_GAMEPAD_BUTTON_DPAD_UP }, { "DPadRight", GLFW_GAMEPAD_BUTTON_DPAD_RIGHT },
	{ "DPadDown", GLFW_GAMEPAD_BUTTON_DPAD_DOWN }, { "DPadLeft", GLFW_GAMEPAD_BUTTON_DPAD_LEFT },
};

static const InputName s_GamepadAxisNames[] =
{
	{ "LeftStickX", GLFW_GAMEPAD_AXIS_LEFT_X }, { "LeftStickY", GLFW_GAMEPAD_AXIS_LEFT_Y }, { "RightStickX", GLFW_GAMEPAD_AXIS_RIGHT_X },
	{ "RightStickY", GLFW_GAMEPAD_AXIS_RIGHT_Y }, { "LeftTrigger", GLFW_GAMEPAD_AXIS_LEFT_TRIGGER },
	{ "RightTrigger", GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER },
};

static const InputName s_GamepadStickNames[] = { { "LeftStick", 0 }, { "RightStick", 1 } };

template<size_t N>
static int FindCode(const InputName (&names)[N], const std::string& name)
{
	for (const InputName& entry : names)
	{
		if (name == entry.Name)
		{
			return entry.Code;
		}
	}
	return -1;
}

template<size_t N>
static const char* FindName(const InputName (&names)[N], int code)
{
	for (const InputName& entry : names)
	{
		if (code == entry.Code)
		{
			return entry.Name;
		}
	}
	return nullptr;
}

int InputMap::FindKey(const std::string& name)
{
	// Letters and digits are their own names (GLFW's codes are their ASCII codes)
	if (name.size() == 1 && ((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= '0' && name[0] <= '9')))
	{
		return name[0];
	}
	if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z')
	{
		return name[0] - 'a' + 'A';
	}
	if (name.size() >= 2 && name[0] == 'F' && name.find_first_not_of("0123456789", 1) == std::string::npos)
	{
		const int number = std::stoi(name.substr(1));
		return (number >= 1 && number <= 12) ? GLFW_KEY_F1 + number - 1 : -1;
	}
	if (name.size() == 7 && name.compare(0, 6, "Keypad") == 0 && name[6] >= '0' && name[6] <= '9')
	{
		return GLFW_KEY_KP_0 + (name[6] - '0');
	}
	return FindCode(s_KeyNames, name);
}

const char* InputMap::GetKeyName(int key)
{
	static const char* letters[] = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
		"T", "U", "V", "W", "X", "Y", "Z" };
	static const char* digits[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	static const char* functionKeys[] = { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" };
	static const char* keypad[] = { "Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4", "Keypad5", "Keypad6", "Keypad7",
		"Keypad8", "Keypad9" };
	if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)   return letters[key - GLFW_KEY_A];
	if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)   return digits[key - GLFW_KEY_0];
	if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) return functionKeys[key - GLFW_KEY_F1];
	if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return keypad[key - GLFW_KEY_KP_0];
	const char* name = FindName(s_KeyNames, key);
	return name ? name : "?";
}

const char* InputMap::GetTypeName(InputActionType type)
{
	switch (type)
	{
		case InputActionType::Button: return "Button";
		case InputActionType::Axis1D: return "Axis1D";
		case InputActionType::Axis2D: return "Axis2D";
	}
	return "?";
}

const InputAction* InputMap::Find(const std::string& name) const
{
	for (const InputAction& action : Actions)
	{
		if (action.Name == name)
		{
			return &action;
		}
	}
	return nullptr;
}

bool InputMap::BindsKey(int key) const
{
	for (const InputAction& action : Actions)
	{
		for (const InputBinding& binding : action.Bindings)
		{
			if (binding.Source == InputBindingSource::Key ? binding.Code == key :
				binding.Source == InputBindingSource::Composite && (binding.Up == key || binding.Down == key || binding.Left == key || binding.Right == key))
			{
				return true;
			}
		}
	}
	return false;
}

// ---- Reading and writing

// A key of a composite: -1 when the node doesn't name one; false when it names a key that doesn't exist
static bool ReadCompositeKey(const YAML::Node& node, const char* field, int& key, std::string& error)
{
	if (!node[field])
	{
		return true;
	}
	const std::string name = node[field].as<std::string>();
	key = InputMap::FindKey(name);
	if (key < 0)
	{
		error = "no key is called '" + name + "'";
		return false;
	}
	return true;
}

// One binding from its node. False (error explains) when it isn't one
static bool ReadBinding(const YAML::Node& node, InputBinding& binding, std::string& error)
{
	if (!node.IsMap())
	{
		error = "a binding is a map, like { Key: Space }";
		return false;
	}
	if (const YAML::Node key = node["Key"])
	{
		binding.Source = InputBindingSource::Key;
		binding.Code = InputMap::FindKey(key.as<std::string>());
		binding.Text = "Key: " + key.as<std::string>();
		if (binding.Code < 0)
		{
			error = "no key is called '" + key.as<std::string>() + "'";
			return false;
		}
	}
	else if (const YAML::Node composite = node["Composite"])
	{
		binding.Source = InputBindingSource::Composite;
		binding.Text = "Composite: " + composite.as<std::string>();
		if (!ReadCompositeKey(node, "Up", binding.Up, error) || !ReadCompositeKey(node, "Down", binding.Down, error) ||
			!ReadCompositeKey(node, "Left", binding.Left, error) || !ReadCompositeKey(node, "Right", binding.Right, error) ||
			!ReadCompositeKey(node, "Negative", binding.Left, error) || !ReadCompositeKey(node, "Positive", binding.Right, error))
		{
			return false;
		}
		if (binding.Up < 0 && binding.Down < 0 && binding.Left < 0 && binding.Right < 0)
		{
			error = "a composite needs keys: Up, Down, Left, Right (or Negative, Positive)";
			return false;
		}
	}
	else if (const YAML::Node mouse = node["Mouse"])
	{
		const std::string name = mouse.as<std::string>();
		binding.Text = "Mouse: " + name;
		if (name == "Delta")
		{
			binding.Source = InputBindingSource::MouseDelta;
		}
		else
		{
			binding.Source = InputBindingSource::MouseButton;
			binding.Code = FindCode(s_MouseNames, name);
			if (binding.Code < 0)
			{
				error = "the mouse has no '" + name + "' (Left, Right, Middle or Delta)";
				return false;
			}
		}
	}
	else if (const YAML::Node gamepad = node["Gamepad"])
	{
		const std::string name = gamepad.as<std::string>();
		binding.Text = "Gamepad: " + name;
		if ((binding.Code = FindCode(s_GamepadButtonNames, name)) >= 0)
		{
			binding.Source = InputBindingSource::GamepadButton;
		}
		else if ((binding.Code = FindCode(s_GamepadAxisNames, name)) >= 0)
		{
			binding.Source = InputBindingSource::GamepadAxis;
		}
		else if ((binding.Code = FindCode(s_GamepadStickNames, name)) >= 0)
		{
			binding.Source = InputBindingSource::GamepadStick;
		}
		else
		{
			error = "a gamepad has no '" + name + "'";
			return false;
		}
	}
	else
	{
		error = "a binding names a Key, a Composite, the Mouse or the Gamepad";
		return false;
	}
	binding.Scale = node["Scale"].as<float>(binding.Scale);
	binding.DeadZone = node["DeadZone"].as<float>(binding.DeadZone);
	return true;
}

bool InputMap::Load(const std::string& filepath, InputMap& map, std::string& error)
{
	YAML::Node root;
	try
	{
		root = YAML::LoadFile(filepath);
	}
	catch (const std::exception& e)
	{
		error = std::string("it can't be read (") + e.what() + ")";
		return false;
	}
	const YAML::Node actions = root["Actions"];
	if (!actions || !actions.IsSequence())
	{
		error = "it is not an input file (it has no Actions)";
		return false;
	}

	map = InputMap();
	try
	{
		for (const YAML::Node& node : actions)
		{
			InputAction action;
			action.Name = node["Name"].as<std::string>("");
			if (action.Name.empty())
			{
				Log::GetLogger()->warn("Input file '{0}': an action without a Name is left out", filepath);
				continue;
			}
			const std::string type = node["Type"].as<std::string>("Button");
			if (type == "Button")      action.Type = InputActionType::Button;
			else if (type == "Axis1D") action.Type = InputActionType::Axis1D;
			else if (type == "Axis2D") action.Type = InputActionType::Axis2D;
			else
			{
				Log::GetLogger()->warn("Input file '{0}': action '{1}' has the type '{2}' (Button, Axis1D or Axis2D): a Button then",
					filepath, action.Name, type);
			}
			if (const YAML::Node bindings = node["Bindings"]; bindings && bindings.IsSequence())
			{
				for (const YAML::Node& bindingNode : bindings)
				{
					InputBinding binding;
					std::string bindingError;
					if (ReadBinding(bindingNode, binding, bindingError))
					{
						action.Bindings.push_back(std::move(binding));
					}
					else
					{
						Log::GetLogger()->warn("Input file '{0}': a binding of '{1}' is left out: {2}", filepath, action.Name, bindingError);
					}
				}
			}
			map.Actions.push_back(std::move(action));
		}
	}
	catch (const std::exception& e)
	{
		error = std::string("it can't be read (") + e.what() + ")";
		return false;
	}
	return true;
}

// A binding as its node: the names it was read with, or the names of its codes
static void WriteBinding(YAML::Emitter& out, const InputBinding& binding)
{
	out << YAML::Flow << YAML::BeginMap;
	switch (binding.Source)
	{
		case InputBindingSource::Key:
			out << YAML::Key << "Key" << YAML::Value << InputMap::GetKeyName(binding.Code);
			break;
		case InputBindingSource::Composite:
		{
			const size_t colon = binding.Text.find(": ");
			out << YAML::Key << "Composite" << YAML::Value << (colon != std::string::npos ? binding.Text.substr(colon + 2) : "Keys");
			const std::pair<const char*, int> keys[] = { { "Up", binding.Up }, { "Down", binding.Down }, { "Left", binding.Left },
				{ "Right", binding.Right } };
			for (const auto& [field, key] : keys)
			{
				if (key >= 0)
				{
					out << YAML::Key << field << YAML::Value << InputMap::GetKeyName(key);
				}
			}
			break;
		}
		case InputBindingSource::MouseButton:
			out << YAML::Key << "Mouse" << YAML::Value << FindName(s_MouseNames, binding.Code);
			break;
		case InputBindingSource::MouseDelta:
			out << YAML::Key << "Mouse" << YAML::Value << "Delta";
			break;
		case InputBindingSource::GamepadButton:
			out << YAML::Key << "Gamepad" << YAML::Value << FindName(s_GamepadButtonNames, binding.Code);
			break;
		case InputBindingSource::GamepadAxis:
			out << YAML::Key << "Gamepad" << YAML::Value << FindName(s_GamepadAxisNames, binding.Code);
			break;
		case InputBindingSource::GamepadStick:
			out << YAML::Key << "Gamepad" << YAML::Value << FindName(s_GamepadStickNames, binding.Code);
			break;
	}
	if (binding.Scale != 1.0f)
	{
		out << YAML::Key << "Scale" << YAML::Value << binding.Scale;
	}
	const bool gamepadAnalog = binding.Source == InputBindingSource::GamepadAxis || binding.Source == InputBindingSource::GamepadStick;
	if (gamepadAnalog)
	{
		out << YAML::Key << "DeadZone" << YAML::Value << binding.DeadZone;
	}
	out << YAML::EndMap;
}

bool InputMap::Save(const std::string& filepath, std::string& error) const
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Actions" << YAML::Value << YAML::BeginSeq;
	for (const InputAction& action : Actions)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << action.Name;
		out << YAML::Key << "Type" << YAML::Value << GetTypeName(action.Type);
		out << YAML::Key << "Bindings" << YAML::Value << YAML::BeginSeq;
		for (const InputBinding& binding : action.Bindings)
		{
			WriteBinding(out, binding);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;

	std::ofstream file(filepath);
	if (!file)
	{
		error = "'" + filepath + "' can't be written";
		return false;
	}
	file << out.c_str() << "\n";
	return true;
}

InputMap InputMap::Default()
{
	auto key = [](int code, const char* name)
	{
		InputBinding binding;
		binding.Source = InputBindingSource::Key;
		binding.Code = code;
		binding.Text = std::string("Key: ") + name;
		return binding;
	};
	auto composite = [](const char* name, int up, int down, int left, int right)
	{
		InputBinding binding;
		binding.Source = InputBindingSource::Composite;
		binding.Up = up;
		binding.Down = down;
		binding.Left = left;
		binding.Right = right;
		binding.Text = std::string("Composite: ") + name;
		return binding;
	};
	auto gamepad = [](InputBindingSource source, int code, const char* name, float scale = 1.0f)
	{
		InputBinding binding;
		binding.Source = source;
		binding.Code = code;
		binding.Scale = scale;
		binding.Text = std::string("Gamepad: ") + name;
		return binding;
	};
	InputBinding mouseDelta;
	mouseDelta.Source = InputBindingSource::MouseDelta;
	mouseDelta.Scale = 0.1f;
	mouseDelta.Text = "Mouse: Delta";

	InputMap map;
	map.Actions.push_back({ "Move", InputActionType::Axis2D, {
		composite("WASD", GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D),
		composite("Arrows", GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT),
		gamepad(InputBindingSource::GamepadStick, 0, "LeftStick") } });
	map.Actions.push_back({ "Look", InputActionType::Axis2D, {
		mouseDelta,
		gamepad(InputBindingSource::GamepadStick, 1, "RightStick", 180.0f) } });
	map.Actions.push_back({ "Jump", InputActionType::Button, {
		key(GLFW_KEY_SPACE, "Space"),
		gamepad(InputBindingSource::GamepadButton, GLFW_GAMEPAD_BUTTON_A, "A") } });
	return map;
}
