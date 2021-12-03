/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "BaseH2M.h"
#include "LayerH2M.h"

#include <vector>


namespace H2M
{

	class LayerStackH2M
	{
	public:
		LayerStackH2M() = default;
		~LayerStackH2M();

		void PushLayer(LayerH2M* layer);
		void PushOverlay(LayerH2M* overlay);
		void PopLayer(LayerH2M* layer);
		void PopOverlay(LayerH2M* overlay);

		std::vector<LayerH2M*>::iterator begin() { return m_Layers.begin(); }
		std::vector<LayerH2M*>::iterator end() { return m_Layers.end(); }
		std::vector<LayerH2M*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<LayerH2M*>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<LayerH2M*>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<LayerH2M*>::const_iterator end() const { return m_Layers.end(); }
		std::vector<LayerH2M*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<LayerH2M*>::const_reverse_iterator rend() const { return m_Layers.rend(); }

	private:
		std::vector<LayerH2M*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}
