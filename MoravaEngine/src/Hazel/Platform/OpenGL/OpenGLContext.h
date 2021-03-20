#pragma once

#include "Hazel/Renderer/RendererContext.h"


struct GLFWwindow;

namespace Hazel {

	class OpenGLContext : public RendererContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);
		virtual ~OpenGLContext();

		virtual void Create() override;
		virtual void BeginFrame() override {}
		virtual void SwapBuffers() override;
		virtual void OnResize(uint32_t width, uint32_t height) override {}
	private:
		GLFWwindow* m_WindowHandle;
	};

}
