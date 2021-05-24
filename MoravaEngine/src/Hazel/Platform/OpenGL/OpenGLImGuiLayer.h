#pragma once

#include "Hazel/Core/Layer.h"

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>


namespace Hazel {

	class OpenGLImGuiLayer : public Layer
	{
	public:
		OpenGLImGuiLayer();
		OpenGLImGuiLayer(const std::string& name);
		~OpenGLImGuiLayer();

		virtual void Begin();
		virtual void End();

		virtual void OnAttach();
		virtual void OnDetach();
		virtual void OnImGuiRender();
	
	private:
		float m_Time = 0.0f;

	};

}
