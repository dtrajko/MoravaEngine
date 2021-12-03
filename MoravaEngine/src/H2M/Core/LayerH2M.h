#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/TimestepH2M.h"
#include "H2M/Core/Events/EventH2M.h"

#include <string>


namespace H2M
{

	class LayerH2M
	{
	public:
		LayerH2M(const std::string& name = "Layer");
		virtual ~LayerH2M();

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUpdate(TimestepH2M ts) = 0;
		virtual void OnEvent(EventH2M& event) = 0;
		virtual void OnRender() = 0;
		virtual void OnImGuiRender() = 0;

		inline const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
	};

}
