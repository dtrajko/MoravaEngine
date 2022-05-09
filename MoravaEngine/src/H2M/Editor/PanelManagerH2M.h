/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/HashH2M.h"
#include "H2M/Editor/EditorPanelH2M.h"

#include <unordered_map>


namespace H2M
{

	struct PanelDataH2M
	{
		const char* ID = "";
		const char* Name = "";
		RefH2M<EditorPanelH2M> Panel = RefH2M<EditorPanelH2M>();
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

		template<typename TPanel>
		RefH2M<TPanel> AddPanel(const PanelDataH2M& panelData)
		{
			static_assert(std::is_base_of<EditorPanelH2M, TPanel>::value, "PanelManager::AddPanel requires TPanel to inherit from EditorPanel");

			uint32_t id = HashH2M::GenerateFNVHash(panelData.ID);
			if (m_Panels.find(id) != m_Panels.end())
			{
				H2M_CORE_ERROR("[PanelManager]: A panel with the id '{0}' has already been added.", panelData.ID);
				return RefH2M<TPanel>();
			}

			m_Panels[id] = panelData;
			return panelData.Panel.As<TPanel>();
		}

		template<typename TPanel, typename... TArgs>
		RefH2M<TPanel> AddPanel(const char* strID, bool isOpenByDefault, TArgs&&... args)
		{
			return AddPanel<TPanel>(PanelData{ strID, strID, Ref<TPanel>::Create(std::forward<TArgs>(args)...), isOpenByDefault });
		}

		template<typename TPanel, typename... TArgs>
		RefH2M<TPanel> AddPanel(const char* strID, const char* displayName, bool isOpenByDefault, TArgs&&... args)
		{
			return AddPanel<TPanel>(PanelDataH2M{ strID, displayName, RefH2M<TPanel>::Create(std::forward<TArgs>(args)...), isOpenByDefault });
		}

	private:
		std::unordered_map<uint32_t, PanelDataH2M> m_Panels;

	};

}
