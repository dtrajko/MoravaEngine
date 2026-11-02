// The Console panel (docs/engine/Game_Engine_Runtime_Guide.html, 4.7): the log in the editor, as the standard output has
// it (both loggers, from the first line on), with the game's messages first: the engine's own details (MORAVA_CORE's
// messages: descriptor sets, buffers, shaders; anything at the trace level) only when asked for, warnings and errors
// always. A message that names a file and a line (a compiler error of the scripts, a script's exception with its stack
// trace) opens it there in the code editor when double-clicked.


#include "Core/Log.h"

#include "spdlog/sinks/base_sink.h"

#include "imgui.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>


#include "EditorState.h"


bool s_ShowConsolePanel = false;

struct ConsoleEntry
{
	char Time[12] = {};
	spdlog::level::level_enum Level = spdlog::level::info;
	bool Engine = false; // MORAVA_CORE's
	std::string Text;
	std::string File;    // the file the message names (empty: none), as written in it (a project's file may be relative)
	int Line = 0;
};

static constexpr size_t s_MaxEntries = 20000;

static std::mutex s_EntriesMutex; // the log writes from any thread
static std::deque<ConsoleEntry> s_Entries;
static uint64_t s_EntriesAdded = 0; // also counts the dropped ones: the selection and the scroll follow it

static bool s_ShowMessages = true, s_ShowWarnings = true, s_ShowErrors = true;
static bool s_ShowEngineDetails = false;
static bool s_ClearOnPlay = false;
static char s_Filter[128] = {};
static uint64_t s_Selected = UINT64_MAX; // in s_EntriesAdded's count
static uint64_t s_SeenEntries = 0;
static bool s_ScrollToEnd = true;

// "Tests.cs(12,5): error CS1002: ; expected [..]": file and line before "): error" or "): warning"
static bool FindCompilerSource(const std::string& text, std::string& file, int& line)
{
	size_t marker = text.find("): error ");
	if (marker == std::string::npos)
	{
		marker = text.find("): warning ");
	}
	if (marker == std::string::npos)
	{
		return false;
	}
	const size_t open = text.rfind('(', marker);
	if (open == std::string::npos || open == 0)
	{
		return false;
	}
	int column = 0;
	if (std::sscanf(text.c_str() + open, "(%d,%d)", &line, &column) < 1)
	{
		return false;
	}
	size_t start = 0;
	while (start < open && text[start] == ' ')
	{
		start++;
	}
	file = text.substr(start, open - start);
	return !file.empty();
}

// A stack trace's first frame with a file: "   at Sample.Coin.OnTriggerEnter(Entity other) in D:\Game\Coin.cs:line 14"
static bool FindStackTraceSource(const std::string& text, std::string& file, int& line)
{
	const size_t lineMarker = text.find(":line ");
	if (lineMarker == std::string::npos)
	{
		return false;
	}
	const size_t in = text.rfind(" in ", lineMarker);
	if (in == std::string::npos)
	{
		return false;
	}
	file = text.substr(in + 4, lineMarker - (in + 4));
	line = std::atoi(text.c_str() + lineMarker + 6);
	return !file.empty() && line > 0;
}

class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& message) override
	{
		ConsoleEntry entry;
		const std::time_t time = std::chrono::system_clock::to_time_t(message.time);
		std::tm local {};
#if defined(_WIN32)
		localtime_s(&local, &time);
#else
		localtime_r(&time, &local);
#endif
		std::strftime(entry.Time, sizeof(entry.Time), "%H:%M:%S", &local);
		entry.Level = message.level;
		entry.Engine = message.logger_name == spdlog::string_view_t("MORAVA_CORE");
		entry.Text.assign(message.payload.data(), message.payload.size());
		if (!FindCompilerSource(entry.Text, entry.File, entry.Line))
		{
			entry.File.clear();
			entry.Line = 0;
			FindStackTraceSource(entry.Text, entry.File, entry.Line);
		}

		std::lock_guard<std::mutex> lock(s_EntriesMutex);
		s_Entries.push_back(std::move(entry));
		s_EntriesAdded++;
		if (s_Entries.size() > s_MaxEntries)
		{
			s_Entries.pop_front();
		}
	}

	void flush_() override {}
};

void Editor::InstallConsole()
{
	Log::AddSink(std::make_shared<ConsoleSink>());
}

void ClearConsole()
{
	std::lock_guard<std::mutex> lock(s_EntriesMutex);
	s_Entries.clear();
	s_Selected = UINT64_MAX;
}

void ClearConsoleIfAskedOnPlay()
{
	if (s_ClearOnPlay)
	{
		ClearConsole();
	}
}

enum class ConsoleKind { Message, Warning, Error };

static ConsoleKind GetKind(spdlog::level::level_enum level)
{
	return level >= spdlog::level::err ? ConsoleKind::Error : level == spdlog::level::warn ? ConsoleKind::Warning : ConsoleKind::Message;
}

// The engine's details: what MORAVA_CORE says (not its warnings and errors), and anything at the trace and debug levels
// (the allocator's "allocating 2048 bytes")
static bool IsDetail(const ConsoleEntry& entry)
{
	return entry.Level < spdlog::level::info || (entry.Engine && GetKind(entry.Level) == ConsoleKind::Message);
}

static bool IsShown(const ConsoleEntry& entry, const std::string& filter)
{
	const ConsoleKind kind = GetKind(entry.Level);
	if (IsDetail(entry) && !s_ShowEngineDetails)
	{
		return false;
	}
	if ((kind == ConsoleKind::Message && !s_ShowMessages) || (kind == ConsoleKind::Warning && !s_ShowWarnings) ||
		(kind == ConsoleKind::Error && !s_ShowErrors))
	{
		return false;
	}
	if (filter.empty())
	{
		return true;
	}
	// Case-insensitive, ASCII
	const auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; };
	const std::string& text = entry.Text;
	for (size_t i = 0; i + filter.size() <= text.size(); i++)
	{
		size_t j = 0;
		while (j < filter.size() && lower(text[i + j]) == filter[j])
		{
			j++;
		}
		if (j == filter.size())
		{
			return true;
		}
	}
	return false;
}

static ImVec4 GetColor(ConsoleKind kind)
{
	switch (kind)
	{
		case ConsoleKind::Error:   return ImVec4(1.0f, 0.42f, 0.38f, 1.0f);
		case ConsoleKind::Warning: return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
		default:                   return ImGui::GetStyleColorVec4(ImGuiCol_Text);
	}
}

// A toggle with the count of its kind: "Errors 3"
static void KindToggle(const char* label, bool& shown, size_t count, ConsoleKind kind)
{
	char text[64];
	std::snprintf(text, sizeof(text), "%s %zu###%s", label, count, label);
	ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(shown ? ImGuiCol_ButtonActive : ImGuiCol_FrameBg));
	if (count > 0 && kind != ConsoleKind::Message)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, GetColor(kind));
	}
	if (ImGui::SmallButton(text))
	{
		shown = !shown;
	}
	if (count > 0 && kind != ConsoleKind::Message)
	{
		ImGui::PopStyleColor();
	}
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(shown ? "Shown: click to hide them" : "Hidden: click to show them");
	}
}

void OnImGuiRenderConsolePanel()
{
	if (!s_ShowConsolePanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(760.0f, 300.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Console", &s_ShowConsolePanel))
	{
		ImGui::End();
		return;
	}

	std::lock_guard<std::mutex> lock(s_EntriesMutex);

	size_t messages = 0, warnings = 0, errors = 0;
	for (const ConsoleEntry& entry : s_Entries)
	{
		const ConsoleKind kind = GetKind(entry.Level);
		if (kind == ConsoleKind::Error)
		{
			errors++;
		}
		else if (kind == ConsoleKind::Warning)
		{
			warnings++;
		}
		else if (!IsDetail(entry) || s_ShowEngineDetails)
		{
			messages++;
		}
	}

	// The toolbar
	if (ImGui::SmallButton("Clear"))
	{
		s_Entries.clear();
		s_Selected = UINT64_MAX;
	}
	ImGui::SameLine();
	KindToggle("Messages", s_ShowMessages, messages, ConsoleKind::Message);
	ImGui::SameLine();
	KindToggle("Warnings", s_ShowWarnings, warnings, ConsoleKind::Warning);
	ImGui::SameLine();
	KindToggle("Errors", s_ShowErrors, errors, ConsoleKind::Error);
	ImGui::SameLine();
	ImGui::Checkbox("Engine Details", &s_ShowEngineDetails);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The engine's own messages (MORAVA_CORE: descriptor sets, buffers, shaders, loading) and anything\n"
			"at the trace level: thousands while a scene opens. Warnings and errors are always shown");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Clear on Play", &s_ClearOnPlay);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x, 80.0f));
	ImGui::InputTextWithHint("##Filter", "Filter", s_Filter, sizeof(s_Filter));

	std::string filter = s_Filter;
	for (char& c : filter)
	{
		c = (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
	}
	const uint64_t firstIndex = s_EntriesAdded - s_Entries.size(); // the s_EntriesAdded count of s_Entries[0]
	std::vector<uint32_t> shown;
	shown.reserve(s_Entries.size());
	for (uint32_t i = 0; i < (uint32_t)s_Entries.size(); i++)
	{
		if (IsShown(s_Entries[i], filter))
		{
			shown.push_back(i);
		}
	}

	// The selected entry in full below the list (copied from there), when there is one
	const ConsoleEntry* selected = s_Selected >= firstIndex && s_Selected < s_EntriesAdded ? &s_Entries[size_t(s_Selected - firstIndex)] : nullptr;
	const float detailsHeight = selected ? ImGui::GetTextLineHeightWithSpacing() * 5.0f : 0.0f;

	ImGui::BeginChild("##Entries", ImVec2(0.0f, -detailsHeight), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	const bool atEnd = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
	ImGuiListClipper clipper;
	clipper.Begin((int)shown.size());
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
		{
			const ConsoleEntry& entry = s_Entries[shown[row]];
			const uint64_t index = firstIndex + shown[row];
			const size_t newline = entry.Text.find('\n');
			const bool more = newline != std::string::npos;
			std::string line = entry.Text.substr(0, newline);
			if (more)
			{
				line += "  (more: select it)";
			}

			ImGui::PushID((int)(index & 0x7fffffff));
			ImGui::PushStyleColor(ImGuiCol_Text, GetColor(GetKind(entry.Level)));
			const std::string label = std::string(entry.Time) + "  " + line;
			if (ImGui::Selectable(label.c_str(), s_Selected == index, ImGuiSelectableFlags_AllowDoubleClick))
			{
				s_Selected = index;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !entry.File.empty())
				{
					OpenScriptFile(entry.File, entry.Line);
				}
			}
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered() && !entry.File.empty())
			{
				ImGui::SetTooltip("Double-click: %s, line %d, in the code editor", entry.File.c_str(), entry.Line);
			}
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Copy"))
				{
					ImGui::SetClipboardText(entry.Text.c_str());
				}
				if (ImGui::MenuItem("Open in the Code Editor", nullptr, false, !entry.File.empty()))
				{
					OpenScriptFile(entry.File, entry.Line);
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
	}
	// New entries: followed when the list was at its end
	if (s_EntriesAdded != s_SeenEntries)
	{
		s_SeenEntries = s_EntriesAdded;
		if (atEnd || s_ScrollToEnd)
		{
			ImGui::SetScrollHereY(1.0f);
		}
		s_ScrollToEnd = false;
	}
	ImGui::EndChild();

	if (selected)
	{
		// Read-only text: it can be selected and copied
		ImGui::InputTextMultiline("##Details", const_cast<char*>(selected->Text.c_str()), selected->Text.size() + 1,
			ImVec2(-1.0f, detailsHeight - ImGui::GetStyle().ItemSpacing.y), ImGuiInputTextFlags_ReadOnly);
	}
	ImGui::End();
}
