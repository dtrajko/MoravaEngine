/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WAR NINGS

#pragma once

#include "H2M/Core/LayerH2M.h"


namespace H2M
{

	class ImGuiLayerH2M : public LayerH2M
	{
	public:
		ImGuiLayerH2M();
		ImGuiLayerH2M(const std::string& name);
		virtual ~ImGuiLayerH2M();

		virtual void Begin() = 0;
		virtual void End() = 0;

		void SetDarkThemeColors();

		static ImGuiLayerH2M* Create();

	};

}
