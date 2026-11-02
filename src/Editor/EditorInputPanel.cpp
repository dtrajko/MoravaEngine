// The Input panel (docs/engine/Game_Engine_Runtime_Guide.html, 2.3): every action of the game with its live value and the
// binding that gave it, and the connected gamepads. "Why doesn't my character move?" at a glance.


#include "Project/Project.h"

#include "imgui.h"

#include <cstdio>


#include "EditorState.h"


// Without a game: the actions the game would get (read again when the project changes, or on Reload), and an input that
// samples them while the panel has the focus (Try), to check the bindings and a controller without Play
static InputMap s_PreviewMap;
static std::string s_PreviewSource = "\n"; // the input file the preview was read from ("": the defaults; "\n": not read)
static InputSystem s_TryInput;
static bool s_Trying = false;

static std::string GetInputSource()
{
	const Project* project = Project::GetOpen();
	return project ? project->GetInputMapPath() : std::string();
}

// An Axis2D value as a dot in a square (the stick's view): x right, y up, the square's edge at 1
static void Axis2DView(const glm::vec2& value, float size)
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + size, min.y + size);
	const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_FrameBg));
	drawList->AddLine(ImVec2(center.x, min.y), ImVec2(center.x, max.y), ImGui::GetColorU32(ImGuiCol_Border));
	drawList->AddLine(ImVec2(min.x, center.y), ImVec2(max.x, center.y), ImGui::GetColorU32(ImGuiCol_Border));
	const glm::vec2 shown = glm::clamp(value, glm::vec2(-1.0f), glm::vec2(1.0f)); // a mouse's or a scaled value goes past 1
	const ImVec2 dot(center.x + shown.x * size * 0.5f, center.y - shown.y * size * 0.5f);
	drawList->AddCircleFilled(dot, 3.0f, ImGui::GetColorU32(value != glm::vec2(0.0f) ? ImGuiCol_PlotHistogram : ImGuiCol_TextDisabled));
	ImGui::Dummy(ImVec2(size, size));
}

static void ActionsTable(const InputMap& map, const InputSystem* input)
{
	if (map.Actions.empty())
	{
		ImGui::TextDisabled("The input file has no actions.");
		return;
	}
	const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp;
	if (!ImGui::BeginTable("Actions", 4, flags))
	{
		return;
	}
	ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 1.0f);
	ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 7.0f);
	ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthStretch, 1.2f);
	ImGui::TableSetupColumn("Bindings", ImGuiTableColumnFlags_WidthStretch, 1.6f);
	ImGui::TableHeadersRow();

	const float squareSize = ImGui::GetFontSize() * 2.5f;
	for (size_t a = 0; a < map.Actions.size(); a++)
	{
		const InputAction& action = map.Actions[a];
		const InputActionState* state = input && a < input->GetStates().size() ? &input->GetStates()[a] : nullptr;
		ImGui::PushID((int)a);
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(action.Name.c_str());
		ImGui::TextDisabled("%s", InputMap::GetTypeName(action.Type));

		ImGui::TableNextColumn();
		if (!state)
		{
			ImGui::TextDisabled("-");
		}
		else if (action.Type == InputActionType::Button)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, state->Held ? ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram) : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_Button));
			ImGui::Button(state->Held ? "Held" : "Up", ImVec2(-FLT_MIN, 0.0f));
			ImGui::PopStyleColor(3);
		}
		else if (action.Type == InputActionType::Axis1D)
		{
			char text[32];
			snprintf(text, sizeof(text), "%.2f", state->Value.x);
			ImGui::ProgressBar(glm::clamp(state->Value.x * 0.5f + 0.5f, 0.0f, 1.0f), ImVec2(-FLT_MIN, 0.0f), text);
		}
		else
		{
			Axis2DView(state->Value, squareSize);
			ImGui::SameLine();
			ImGui::Text("%.2f\n%.2f", state->Value.x, state->Value.y);
		}

		ImGui::TableNextColumn();
		if (state && state->Binding >= 0 && state->Binding < (int)action.Bindings.size())
		{
			ImGui::TextUnformatted(action.Bindings[state->Binding].Text.c_str());
		}
		else
		{
			ImGui::TextDisabled("-");
		}

		ImGui::TableNextColumn();
		for (const InputBinding& binding : action.Bindings)
		{
			ImGui::TextDisabled("%s", binding.Text.c_str());
		}
		if (action.Bindings.empty())
		{
			ImGui::TextDisabled("(no bindings: always 0)");
		}
		ImGui::PopID();
	}
	ImGui::EndTable();
}

static void GamepadsList()
{
	ImGui::SeparatorText("Gamepads");
	const std::vector<InputGamepadInfo> gamepads = InputSystem::GetGamepads();
	if (gamepads.empty())
	{
		ImGui::TextDisabled("None connected (plugging one in works at any time)");
		return;
	}
	for (const InputGamepadInfo& gamepad : gamepads)
	{
		if (gamepad.Mapped)
		{
			ImGui::BulletText("%d: %s", gamepad.Index + 1, gamepad.Name.c_str());
		}
		else
		{
			ImGui::BulletText("%d: %s (no layout known: not read)", gamepad.Index + 1, gamepad.Name.c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("GLFW knows the layouts of most controllers (from SDL's database of controllers); this one\n"
					"isn't among them, so its buttons and sticks have no names, and no binding reads it.");
			}
		}
	}
}

void OnImGuiRenderInputPanel()
{
	if (!s_ShowInputPanel)
	{
		s_Trying = false;
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Input", &s_ShowInputPanel))
	{
		ImGui::End();
		return;
	}

	const std::string source = GetInputSource();
	const InputSystem* gameInput = GetGameInput();
	if (gameInput)
	{
		// The running game's actions and values
		s_Trying = false;
		if (!gameInput->IsEnabled())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.35f, 1.0f), IsGameRunning() ? "The game has no input: click the viewport" : "Paused: the game has no input");
		}
		else
		{
			ImGui::Text("The game has the input%s", gameInput->IsMouseCaptured() ? ", and the mouse (captured)" : "");
		}
		ActionsTable(gameInput->GetMap(), gameInput);
	}
	else
	{
		bool reload = source != s_PreviewSource;
		if (reload)
		{
			s_PreviewSource = source;
		}
		ImGui::TextWrapped("%s", source.empty() ? "The default actions (no project, or the project has no input file)" : source.c_str());
		if (ImGui::IsItemHovered() && !source.empty())
		{
			ImGui::SetTooltip("The project's input file (YAML): edit it in a text editor, then Reload. Play reads it again each time.");
		}
		if (ImGui::Button("Reload"))
		{
			reload = true;
		}
		if (reload)
		{
			s_PreviewMap = GameSystems::LoadInputMap();
			s_TryInput.SetMap(s_PreviewMap);
		}
		ImGui::SameLine();
		ImGui::Checkbox("Try", &s_Trying);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The actions take their values while this panel has the focus, without Play:\n"
				"press the keys, move the mouse, try a controller");
		}
		if (s_Trying)
		{
			const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
			s_TryInput.Update(GetCurrentPlatformWindow(), focused, ImGui::GetIO().DeltaTime);
		}
		ActionsTable(s_PreviewMap, s_Trying ? &s_TryInput : nullptr);
	}

	GamepadsList();
	ImGui::End();
}
