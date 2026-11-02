#pragma once

#include <unordered_map>


namespace H2M
{

	struct PanelDataH2M
	{
		const char* ID = "";
		const char* Name = "";
		// RefH2M<EditorPanelH2M> Panel = nullptr;
		bool IsOpen = false;
	};

	class PanelManagerH2M
	{
	public:
		PanelManagerH2M() = default;
		~PanelManagerH2M()
		{
			m_Panels.clear();
		}

	private:
		std::unordered_map<uint32_t, PanelDataH2M> m_Panels;

	};

}
