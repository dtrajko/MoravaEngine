/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/Events/EventH2M.h"
#include "H2M/Core/RefH2M.h"
#include "H2M/Scene/SceneH2M.h"


namespace H2M
{

	class EditorPanelH2M : public RefCountedH2M
	{
	public:
		virtual void OnImGuiRender(bool& isOpen) = 0;
		virtual void OnEvent(EventH2M& e) {}
		// virtual void OnProjectChanged(const RefH2M<ProjectH2M>& project) {}
		virtual void SetSceneContext(const RefH2M<SceneH2M>& context) {}
	};

}
