/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/RendererContextH2M.h"

#include "Core/Window.h"


struct GLFWwindow;

namespace H2M
{

	class OpenGLContextH2M : public RendererContextH2M
	{
	public:
		OpenGLContextH2M(Window* window);
		virtual ~OpenGLContextH2M();

		virtual void Init() override;

		// virtual void Create() override;                                 // TODO: remove the method
		virtual void SwapBuffers() override;                               // TODO: remove the method
		virtual void OnResize(uint32_t width, uint32_t height) override {} // TODO: remove the method
		virtual void BeginFrame() override {}                              // TODO: remove the method

	private:
		Window* m_Window;

	};

}
