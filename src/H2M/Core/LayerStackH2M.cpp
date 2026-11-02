/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "LayerStackH2M.h"


namespace H2M
{

	LayerStackH2M::~LayerStackH2M()
	{
		for (LayerH2M* layer : m_Layers)
			delete layer;
	}

	void LayerStackH2M::PushLayer(LayerH2M* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
	}

	void LayerStackH2M::PushOverlay(LayerH2M* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStackH2M::PopLayer(LayerH2M* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStackH2M::PopOverlay(LayerH2M* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
			m_Layers.erase(it);
	}

}
