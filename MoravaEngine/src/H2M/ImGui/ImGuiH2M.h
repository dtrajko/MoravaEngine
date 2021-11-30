#pragma once

#include "ImGui/ImGuiWrapper.h"

#include <string>


namespace H2M::UI {

	static uint32_t s_CounterLegacy = 0;
	static char s_IDBufferLegacy[16];

	static bool PropertyH2M(const char* label, std::string& value, bool error = false)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy(buffer, value.c_str());

		s_IDBufferLegacy[0] = '#';
		s_IDBufferLegacy[1] = '#';
		memset(s_IDBufferLegacy + 2, 0, 14);
		// _itoa_s(s_CounterLegacy++, s_IDBufferLegacy + 2, 16, 16);
		sprintf(s_IDBufferLegacy + 2, "%d", s_CounterLegacy++);

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (ImGui::InputText(s_IDBufferLegacy, buffer, 256))
		{
			value = buffer;
			modified = true;
		}
		if (error)
			ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

}
